// connection_thread.h

#ifndef CONNECTION_THREAD_H
#define CONNECTION_THREAD_H

#include <pthread.h>

struct thread_args {
    int client_socket;
    const char *root_directory;
};

void *connection_thread(void *arg);

#endif // CONNECTION_THREAD_H
