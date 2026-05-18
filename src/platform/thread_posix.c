/* thread_posix.c — pthreads implementation */

#include "thread.h"
#include <pthread.h>
#include <stdlib.h>

int plat_thread_spawn(plat_thread_fn fn, void *arg, int stack_size) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_size > 0)
        pthread_attr_setstacksize(&attr, (size_t)stack_size);

    pthread_t tid;
    int r = pthread_create(&tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    if (r == 0) pthread_detach(tid);
    return r == 0 ? 0 : -1;
}

void plat_mutex_init(plat_mutex_t *m)    { pthread_mutex_init(&m->m, NULL); }
void plat_mutex_lock(plat_mutex_t *m)    { pthread_mutex_lock(&m->m); }
void plat_mutex_unlock(plat_mutex_t *m)  { pthread_mutex_unlock(&m->m); }
