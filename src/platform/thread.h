/*
 * thread.h — portable threading and mutex
 */
#ifndef PLATFORM_THREAD_H
#define PLATFORM_THREAD_H

#include "platform.h"

typedef void *(*plat_thread_fn)(void *);

/* Spawn a detached thread. Returns 0 on success. */
int plat_thread_spawn(plat_thread_fn fn, void *arg, int stack_size);

/* Mutex */
#ifdef PLAT_WINDOWS
typedef struct { CRITICAL_SECTION cs; } plat_mutex_t;
#else
#include <pthread.h>
typedef struct { pthread_mutex_t m; } plat_mutex_t;
#endif

void plat_mutex_init(plat_mutex_t *m);
void plat_mutex_lock(plat_mutex_t *m);
void plat_mutex_unlock(plat_mutex_t *m);

#endif /* PLATFORM_THREAD_H */
