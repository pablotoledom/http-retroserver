// connection_thread.c

#include "connection_thread.h"
#include "connection.h"
#include "request_handler.h"
#include "../utils/log.h"
#include "../platform/platform.h"
#include <stdlib.h>
#include <string.h>

#ifndef PLAT_WINDOWS
#  include <sys/socket.h>
#  include <sys/time.h>
#endif

extern void unregister_connection(void);

void *connection_thread(void *arg) {
    SIGPIPE_IGNORE();

    struct thread_args *args = (struct thread_args *)arg;
    if (!args) { unregister_connection(); return NULL; }

    connection_ctx_t *conn = (connection_ctx_t *)malloc(sizeof(*conn));
    if (!conn) {
        LOG_ERROR("Failed to allocate connection context");
        plat_sock_close(args->client_socket);
        free(args);
        unregister_connection();
        return NULL;
    }

    conn->client_socket = args->client_socket;

#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt((int)conn->client_socket, SOL_SOCKET, SO_NOSIGPIPE,
               (const char *)&set, sizeof(set));
#endif

#ifdef PLAT_WINDOWS
    DWORD timeout_ms = 10000;
    setsockopt((SOCKET)conn->client_socket, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&timeout_ms, sizeof(timeout_ms));
    setsockopt((SOCKET)conn->client_socket, SOL_SOCKET, SO_SNDTIMEO,
               (const char *)&timeout_ms, sizeof(timeout_ms));
#else
    struct timeval timeout = {10, 0};
    setsockopt(conn->client_socket, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));
    setsockopt(conn->client_socket, SOL_SOCKET, SO_SNDTIMEO,
               &timeout, sizeof(timeout));
#endif

    request_handler(plain_read, conn, args->root_directory);

    connection_close(conn);
    free(conn);
    free(args);
    unregister_connection();
    return NULL;
}
