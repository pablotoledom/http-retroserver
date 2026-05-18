// start_stop.c

#include "start_stop.h"
#include "connection_thread.h"
#include "ssl_manager.h"
#include "../utils/log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>

#define THREAD_STACK_SIZE (2 * 1024 * 1024)

static int server_fd_http  = -1;
static int server_fd_https = -1;
static SSL_CTX *ssl_ctx = NULL;
static volatile int running = 1;

// --- connection limit ---
#define MAX_CONNECTIONS 200
static int active_connections = 0;
static pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- rate limiting ---
#define MAX_IPS     1024
#define RATE_WINDOW    5
#define RATE_LIMIT   500

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
    if (active_connections < MAX_CONNECTIONS) {
        active_connections++;
        allowed = 1;
    }
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
            e->blocked = 0;
            e->count   = 0;
        }
        if (e->blocked) return 1;

        if ((now - e->last_conn) < RATE_WINDOW) {
            e->count++;
            e->last_conn = now;
            if (e->count > RATE_LIMIT) {
                e->blocked = 1;
                e->last_rejected = now;
                LOG_WARN("Blocking IP %s for %d seconds", inet_ntoa(client_ip), RATE_WINDOW * 2);
                return 1;
            }
            return 0;
        } else {
            e->count = 1;
            e->last_conn = now;
            return 0;
        }
    }

    for (int i = 0; i < MAX_IPS; ++i) {
        if (ip_table[i].ip == 0) {
            ip_table[i].ip        = ip;
            ip_table[i].count     = 1;
            ip_table[i].last_conn = now;
            ip_table[i].blocked   = 0;
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
        close(fd);
        return -1;
    }
    if (listen(fd, 128) < 0) {
        LOG_ERROR("listen() failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static void accept_client(int server_fd, SSL_CTX *ctx, const char *root_dir,
                           struct sockaddr_in *addr, socklen_t addrlen) {
    int client = accept(server_fd, (struct sockaddr *)addr, &addrlen);
    if (client < 0) return;

    if (too_many_connections(addr->sin_addr)) {
        LOG_WARN("Rate limit: dropping %s", inet_ntoa(addr->sin_addr));
        close(client);
        return;
    }
    if (!try_register_connection()) {
        LOG_WARN("Connection limit reached, dropping client");
        close(client);
        return;
    }

#ifdef SO_NOSIGPIPE
    int val = 1;
    setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
#endif

    struct thread_args *args = malloc(sizeof(*args));
    if (!args) {
        close(client);
        unregister_connection();
        return;
    }

    args->client_socket = client;
    args->root_directory = root_dir;
    args->ssl = NULL;

    if (ctx) {
        args->ssl = SSL_new(ctx);
        if (!args->ssl) {
            LOG_ERROR("SSL_new failed");
            close(client);
            free(args);
            unregister_connection();
            return;
        }
        SSL_set_fd(args->ssl, client);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);

    pthread_t tid;
    if (pthread_create(&tid, &attr, connection_thread, args) != 0) {
        LOG_ERROR("pthread_create failed");
        if (args->ssl) SSL_free(args->ssl);
        close(client);
        free(args);
        unregister_connection();
    } else {
        pthread_detach(tid);
    }
    pthread_attr_destroy(&attr);
}

int server_start(const char *root_dir, int ssl_enabled,
                 const char *ssl_cert, const char *ssl_key,
                 int http_port, int https_port) {
    signal(SIGPIPE, SIG_IGN);
    running = 1;

    server_fd_http = make_listen_socket(http_port);
    if (server_fd_http < 0) return -1;
    LOG_INFO("HTTP listening on port %d", http_port);

    if (ssl_enabled) {
        server_fd_https = make_listen_socket(https_port);
        if (server_fd_https < 0) return -1;
        ssl_ctx = ssl_create_context(ssl_cert, ssl_key);
        if (!ssl_ctx) return -1;
        LOG_INFO("HTTPS listening on port %d", https_port);
    }

    fd_set readfds;
    int max_sd = server_fd_http;
    if (ssl_enabled && server_fd_https > max_sd) max_sd = server_fd_https;

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(server_fd_http, &readfds);
        if (ssl_enabled && server_fd_https >= 0) FD_SET(server_fd_https, &readfds);

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("select() error: %s", strerror(errno));
            break;
        }

        if (server_fd_http >= 0 && FD_ISSET(server_fd_http, &readfds))
            accept_client(server_fd_http, NULL, root_dir, &addr, addrlen);

        if (ssl_enabled && server_fd_https >= 0 && FD_ISSET(server_fd_https, &readfds))
            accept_client(server_fd_https, ssl_ctx, root_dir, &addr, addrlen);
    }

    return 0;
}

void server_stop(void) {
    running = 0;
    if (server_fd_http  >= 0) { close(server_fd_http);  server_fd_http  = -1; }
    if (server_fd_https >= 0) { close(server_fd_https); server_fd_https = -1; }
    if (ssl_ctx) { ssl_free_context(ssl_ctx); ssl_ctx = NULL; }
}
