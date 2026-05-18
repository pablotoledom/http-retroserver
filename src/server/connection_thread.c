// connection_thread.c

#include "connection_thread.h"
#include "connection.h"
#include "request_handler.h"
#include "../utils/log.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

extern void unregister_connection(void);

void *connection_thread(void *arg) {
    signal(SIGPIPE, SIG_IGN);

    struct thread_args *args = arg;
    if (!args) {
        unregister_connection();
        return NULL;
    }

    connection_ctx_t *conn = malloc(sizeof(*conn));
    if (!conn) {
        LOG_ERROR("Failed to allocate connection context");
        close(args->client_socket);
        free(args);
        unregister_connection();
        return NULL;
    }

    conn->client_socket = args->client_socket;

#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(conn->client_socket, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
#endif

    struct timeval timeout = {10, 0};
    setsockopt(conn->client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(conn->client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    request_handler(plain_read, conn, args->root_directory);

    connection_close(conn);
    free(conn);
    free(args);
    unregister_connection();
    return NULL;
}
