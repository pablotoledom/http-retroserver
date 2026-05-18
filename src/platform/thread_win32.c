/* thread_win32.c — Win32 threads + CRITICAL_SECTION
 *
 * Uses _beginthread() instead of CreateThread() so that MSVCRT.DLL
 * properly initializes per-thread CRT data (malloc, errno, etc.).
 * Required for correct behavior on Windows 95/98.
 */

#include "thread.h"
#include <process.h>   /* _beginthread */
#include <stdlib.h>

typedef struct { plat_thread_fn fn; void *arg; } wrap_t;

static void thread_wrapper(void *p) {
    wrap_t *w = (wrap_t *)p;
    w->fn(w->arg);
    free(w);
    /* _beginthread closes the handle automatically when the thread exits */
}

int plat_thread_spawn(plat_thread_fn fn, void *arg, int stack_size) {
    (void)stack_size;  /* ignored — use PE default to avoid Win95 VM issues */
    wrap_t *w = (wrap_t *)malloc(sizeof(*w));
    if (!w) return -1;
    w->fn = fn; w->arg = arg;

    /* Pass stack_size=0 to use the default from the PE header (~1MB).
     * Requesting a fixed large size (e.g. 2MB) can fail on Win95
     * due to limited virtual memory. */
    uintptr_t h = _beginthread(thread_wrapper, 0, w);
    if (h == (uintptr_t)-1) { free(w); return -1; }
    return 0;
}

void plat_mutex_init(plat_mutex_t *m)   { InitializeCriticalSection(&m->cs); }
void plat_mutex_lock(plat_mutex_t *m)   { EnterCriticalSection(&m->cs); }
void plat_mutex_unlock(plat_mutex_t *m) { LeaveCriticalSection(&m->cs); }
