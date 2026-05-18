// connection.c

#include "connection.h"
#include "../utils/log.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

ssize_t connection_write(void *ctx, const char *buf, size_t count) {
    connection_ctx_t *conn = (connection_ctx_t *)ctx;
    if (!conn) return -1;

    ssize_t sent;
#ifdef MSG_NOSIGNAL
    sent = send(conn->client_socket, buf, count, MSG_NOSIGNAL);
#else
    sent = write(conn->client_socket, buf, count);
#endif
    if (sent > 0) return sent;
    if (sent == 0) return 0;
    if (errno == EPIPE || errno == ECONNRESET) return 0;
    if (errno == EINTR) return -2;
    LOG_ERROR("write/send error (%s)", strerror(errno));
    return -1;
}

void connection_close(void *ctx) {
    if (!ctx) return;
    connection_ctx_t *conn = (connection_ctx_t *)ctx;
    if (conn->client_socket >= 0) {
        shutdown(conn->client_socket, SHUT_RDWR);
        close(conn->client_socket);
        conn->client_socket = -1;
    }
}

ssize_t plain_read(void *ctx, char *buf, size_t count) {
    connection_ctx_t *conn = (connection_ctx_t *)ctx;
    if (!conn) return -1;

    ssize_t ret = read(conn->client_socket, buf, count);
    if (ret > 0) return ret;
    if (ret == 0) return 0;
    if (errno == EINTR) return -2;
    if (errno == EPIPE || errno == ECONNRESET) return 0;
    LOG_ERROR("plain_read: %s", strerror(errno));
    return -1;
}
