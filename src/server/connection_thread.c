// connection_thread.c

#include "connection_thread.h"
#include "connection.h"
#include "request_handler.h"
#include "../utils/log.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>

extern void unregister_connection(void);

static void conn_cleanup(connection_ctx_t *conn, int need_close_socket) {
    if (!conn) return;

    if (conn->ssl) {
        int fd = SSL_get_fd(conn->ssl);
        if (fd >= 0) {
            int s = SSL_shutdown(conn->ssl);
            if (s == 0) s = SSL_shutdown(conn->ssl);
            (void)s;
        }
        SSL_free(conn->ssl);
        conn->ssl = NULL;
    }

    if (need_close_socket && conn->client_socket >= 0) {
        close(conn->client_socket);
        conn->client_socket = -1;
    }

    free(conn);
}

void *connection_thread(void *arg) {
    signal(SIGPIPE, SIG_IGN);

    struct thread_args *args = arg;
    connection_ctx_t *conn = NULL;
    int need_close_socket = 0;

    if (!args) {
        LOG_ERROR("connection_thread: args are NULL");
        unregister_connection();
        return NULL;
    }

    conn = malloc(sizeof(*conn));
    if (!conn) {
        LOG_ERROR("Failed to allocate connection context");
        if (args->ssl) SSL_free(args->ssl);
        close(args->client_socket);
        free(args);
        unregister_connection();
        return NULL;
    }

    conn->client_socket = args->client_socket;
    conn->ssl = args->ssl;
    args->ssl = NULL;
    need_close_socket = 1;

#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(conn->client_socket, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
#endif

    struct timeval timeout = {10, 0};
    setsockopt(conn->client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(conn->client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (conn->ssl) {
        int ret = SSL_accept(conn->ssl);
        if (ret <= 0) {
            int err = SSL_get_error(conn->ssl, ret);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                LOG_WARN("SSL handshake failed");
                goto cleanup;
            }
        }
    }

    request_handler(conn->ssl ? ssl_read : plain_read, conn, args->root_directory);

cleanup:
    conn_cleanup(conn, need_close_socket);
    free(args);
    unregister_connection();
    return NULL;
}
