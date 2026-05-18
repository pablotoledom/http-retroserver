// start_stop.c

#include "start_stop.h"
#include "connection_thread.h"
#include "../utils/log.h"
#include "../platform/platform.h"
#include "../platform/thread.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PLAT_WINDOWS
#  include <arpa/inet.h>
#  include <signal.h>
#  include <sys/select.h>
#endif

#define THREAD_STACK_SIZE (2 * 1024 * 1024)

static plat_sock_t server_fd = PLAT_INVALID_SOCK;
static volatile int running  = 1;

#define MAX_CONNECTIONS 200
static int active_connections = 0;
static plat_mutex_t connection_mutex;

#define MAX_IPS     1024
#define RATE_WINDOW    5
#define RATE_LIMIT   500

typedef struct {
    unsigned long ip;
    time_t last_conn;
    time_t last_rejected;
    int count;
    int blocked;
} ip_entry_t;

static ip_entry_t ip_table[MAX_IPS];

int try_register_connection(void) {
    int allowed = 0;
    plat_mutex_lock(&connection_mutex);
    if (active_connections < MAX_CONNECTIONS) { active_connections++; allowed = 1; }
    plat_mutex_unlock(&connection_mutex);
    return allowed;
}

void unregister_connection(void) {
    plat_mutex_lock(&connection_mutex);
    if (active_connections > 0) active_connections--;
    plat_mutex_unlock(&connection_mutex);
}

static int too_many_connections(unsigned long ip_addr) {
    time_t now = time(NULL);

    for (int i = 0; i < MAX_IPS; ++i) {
        ip_entry_t *e = &ip_table[i];
        if (e->ip != ip_addr) continue;
        if (e->blocked && (now - e->last_rejected) > RATE_WINDOW * 2) {
            e->blocked = 0; e->count = 0;
        }
        if (e->blocked) return 1;
        if ((now - e->last_conn) < RATE_WINDOW) {
            e->count++; e->last_conn = now;
            if (e->count > RATE_LIMIT) {
                e->blocked = 1; e->last_rejected = now;
                LOG_WARN("Rate limiting IP");
                return 1;
            }
            return 0;
        }
        e->count = 1; e->last_conn = now; return 0;
    }
    for (int i = 0; i < MAX_IPS; ++i) {
        if (ip_table[i].ip == 0) {
            ip_table[i].ip = ip_addr; ip_table[i].count = 1;
            ip_table[i].last_conn = now; ip_table[i].blocked = 0;
            return 0;
        }
    }
    return 0;
}

static plat_sock_t make_listen_socket(int port) {
    plat_sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!plat_sock_valid(fd)) {
        LOG_ERROR("socket() failed");
        return PLAT_INVALID_SOCK;
    }

    int opt = 1;
    setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((unsigned short)port);

    if (bind((int)fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind() failed on port %d", port);
        plat_sock_close(fd);
        return PLAT_INVALID_SOCK;
    }
    if (listen((int)fd, 128) < 0) {
        LOG_ERROR("listen() failed");
        plat_sock_close(fd);
        return PLAT_INVALID_SOCK;
    }
    return fd;
}

int server_start(const char *root_dir, int http_port) {
    SIGPIPE_IGNORE();
    running = 1;
    plat_mutex_init(&connection_mutex);

    server_fd = make_listen_socket(http_port);
    if (!plat_sock_valid(server_fd)) return -1;
    LOG_INFO("HTTP listening on port %d", http_port);

    struct sockaddr_in addr;
#ifdef PLAT_WINDOWS
    int addrlen = sizeof(addr);
#else
    socklen_t addrlen = sizeof(addr);
#endif

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

#ifdef PLAT_WINDOWS
        int activity = select(0, &readfds, NULL, NULL, NULL);
#else
        int activity = select((int)server_fd + 1, &readfds, NULL, NULL, NULL);
#endif
        if (activity < 0) {
#ifndef PLAT_WINDOWS
            if (errno == EINTR) continue;
#endif
            break;
        }

        if (!FD_ISSET(server_fd, &readfds)) continue;

        plat_sock_t client = (plat_sock_t)accept((int)server_fd,
                                                  (struct sockaddr *)&addr,
                                                  &addrlen);
        if (!plat_sock_valid(client)) continue;

        if (too_many_connections(ntohl(addr.sin_addr.s_addr))) {
            LOG_WARN("Rate limit: dropping connection");
            plat_sock_close(client); continue;
        }
        if (!try_register_connection()) {
            LOG_WARN("Connection limit reached");
            plat_sock_close(client); continue;
        }

#ifdef SO_NOSIGPIPE
        int val = 1;
        setsockopt((int)client, SOL_SOCKET, SO_NOSIGPIPE,
                   (const char *)&val, sizeof(val));
#endif

        struct thread_args *args = (struct thread_args *)malloc(sizeof(*args));
        if (!args) { plat_sock_close(client); unregister_connection(); continue; }

        args->client_socket  = client;
        args->root_directory = root_dir;

        if (plat_thread_spawn(connection_thread, args, THREAD_STACK_SIZE) != 0) {
            LOG_ERROR("Failed to spawn thread");
            plat_sock_close(client); free(args); unregister_connection();
        }
    }

    return 0;
}

void server_stop(void) {
    running = 0;
    if (plat_sock_valid(server_fd)) {
        plat_sock_close(server_fd);
        server_fd = PLAT_INVALID_SOCK;
    }
}
