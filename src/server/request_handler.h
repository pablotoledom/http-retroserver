// request_handler.h

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <sys/types.h>

typedef ssize_t (*read_func_t)(void *ctx, char *buf, size_t count);

void request_handler(read_func_t read_func, void *ctx, const char *root_directory);

#endif // REQUEST_HANDLER_H
