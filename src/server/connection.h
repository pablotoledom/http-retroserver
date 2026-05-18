// connection.h

#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    int client_socket;
} connection_ctx_t;

ssize_t connection_write(void *ctx, const char *buf, size_t count);
void    connection_close(void *ctx);
ssize_t plain_read(void *ctx, char *buf, size_t count);

#endif // CONNECTION_H
