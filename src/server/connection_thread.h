// connection_thread.h

#ifndef CONNECTION_THREAD_H
#define CONNECTION_THREAD_H

#include "platform/platform.h"

struct thread_args {
    plat_sock_t  client_socket;
    const char  *root_directory;
};

void *connection_thread(void *arg);

#endif // CONNECTION_THREAD_H
