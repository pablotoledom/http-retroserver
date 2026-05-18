// connection.c

#include "connection.h"
#include "../utils/log.h"
#include "../platform/platform.h"
#include <errno.h>
#include <string.h>

#ifndef PLAT_WINDOWS
#  include <unistd.h>
#  include <sys/socket.h>
#endif

ssize_t connection_write(void *ctx, const char *buf, size_t count) {
    connection_ctx_t *conn = (connection_ctx_t *)ctx;
    if (!conn || count == 0) return 0;

    size_t total = 0;
    while (total < count) {
        ssize_t sent = (ssize_t)plat_send(conn->client_socket,
                                           buf + total, count - total);
        if (sent > 0) {
            total += (size_t)sent;
            continue;
        }
        if (sent == 0) break;
#ifdef PLAT_WINDOWS
        int err = WSAGetLastError();
        if (err == WSAECONNRESET || err == WSAEWOULDBLOCK) break;
#else
        if (errno == EINTR)  continue;   /* retry on signal */
        if (errno == EPIPE || errno == ECONNRESET) break;
#endif
        LOG_ERROR("send error");
        break;
    }
    return (ssize_t)total;
}

void connection_close(void *ctx) {
    if (!ctx) return;
    connection_ctx_t *conn = (connection_ctx_t *)ctx;
    if (plat_sock_valid(conn->client_socket)) {
        shutdown(conn->client_socket, 2);  /* SHUT_RDWR = 2 on all platforms */
        plat_sock_close(conn->client_socket);
        conn->client_socket = PLAT_INVALID_SOCK;
    }
}

ssize_t plain_read(void *ctx, char *buf, size_t count) {
    connection_ctx_t *conn = (connection_ctx_t *)ctx;
    if (!conn) return -1;

    ssize_t ret = (ssize_t)plat_recv(conn->client_socket, buf, count);
    if (ret > 0) return ret;
    if (ret == 0) return 0;
#ifndef PLAT_WINDOWS
    if (errno == EINTR) return -2;
    if (errno == EPIPE || errno == ECONNRESET) return 0;
#endif
    LOG_ERROR("recv error");
    return -1;
}
