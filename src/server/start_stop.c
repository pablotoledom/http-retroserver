// start_stop.c

#include "start_stop.h"
#include "connection_thread.h"
#include "../utils/log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

#define THREAD_STACK_SIZE (2 * 1024 * 1024)

static int server_fd = -1;
static volatile int running = 1;

#define MAX_CONNECTIONS 200
static int active_connections = 0;
static pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_IPS      1024
#define RATE_WINDOW     5
#define RATE_LIMIT    500

typedef struct {
    in_addr_t ip;
    time_t last_conn;
    time_t last_rejected;
    int count;
    int blocked;
} ip_entry_t;

static ip_entry_t ip_table[MAX_IPS];

int try_register_connection(void) {
    int allowed = 0;
    pthread_mutex_lock(&connection_mutex);
    if (active_connections < MAX_CONNECTIONS) { active_connections++; allowed = 1; }
    pthread_mutex_unlock(&connection_mutex);
    return allowed;
}

void unregister_connection(void) {
    pthread_mutex_lock(&connection_mutex);
    if (active_connections > 0) active_connections--;
    pthread_mutex_unlock(&connection_mutex);
}

static int too_many_connections(struct in_addr client_ip) {
    time_t now = time(NULL);
    in_addr_t ip = client_ip.s_addr;

    for (int i = 0; i < MAX_IPS; ++i) {
        ip_entry_t *e = &ip_table[i];
        if (e->ip != ip) continue;
        if (e->blocked && (now - e->last_rejected) > RATE_WINDOW * 2) {
            e->blocked = 0; e->count = 0;
        }
        if (e->blocked) return 1;
        if ((now - e->last_conn) < RATE_WINDOW) {
            e->count++; e->last_conn = now;
            if (e->count > RATE_LIMIT) {
                e->blocked = 1; e->last_rejected = now;
                LOG_WARN("Blocking IP %s for %d seconds", inet_ntoa(client_ip), RATE_WINDOW * 2);
                return 1;
            }
            return 0;
        } else { e->count = 1; e->last_conn = now; return 0; }
    }
    for (int i = 0; i < MAX_IPS; ++i) {
        if (ip_table[i].ip == 0) {
            ip_table[i].ip = ip; ip_table[i].count = 1;
            ip_table[i].last_conn = now; ip_table[i].blocked = 0;
            return 0;
        }
    }
    return 0;
}

static int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { LOG_ERROR("socket() failed: %s", strerror(errno)); return -1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind() failed on port %d: %s", port, strerror(errno));
        close(fd); return -1;
    }
    if (listen(fd, 128) < 0) {
        LOG_ERROR("listen() failed: %s", strerror(errno));
        close(fd); return -1;
    }
    return fd;
}

int server_start(const char *root_dir, int http_port) {
    signal(SIGPIPE, SIG_IGN);
    running = 1;

    server_fd = make_listen_socket(http_port);
    if (server_fd < 0) return -1;
    LOG_INFO("HTTP listening on port %d", http_port);

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int activity = select(server_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("select() error: %s", strerror(errno));
            break;
        }

        if (!FD_ISSET(server_fd, &readfds)) continue;

        int client = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client < 0) continue;

        if (too_many_connections(addr.sin_addr)) {
            LOG_WARN("Rate limit: dropping %s", inet_ntoa(addr.sin_addr));
            close(client); continue;
        }
        if (!try_register_connection()) {
            LOG_WARN("Connection limit reached, dropping client");
            close(client); continue;
        }

#ifdef SO_NOSIGPIPE
        int val = 1;
        setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
#endif

        struct thread_args *args = malloc(sizeof(*args));
        if (!args) { close(client); unregister_connection(); continue; }

        args->client_socket  = client;
        args->root_directory = root_dir;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);

        pthread_t tid;
        if (pthread_create(&tid, &attr, connection_thread, args) != 0) {
            LOG_ERROR("pthread_create failed");
            close(client); free(args); unregister_connection();
        } else {
            pthread_detach(tid);
        }
        pthread_attr_destroy(&attr);
    }

    return 0;
}

void server_stop(void) {
    running = 0;
    if (server_fd >= 0) { close(server_fd); server_fd = -1; }
}
