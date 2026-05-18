/*
 * platform.h — OS detection and portable primitives
 *
 * Supported: Linux, macOS, Windows 95+ (WinSock 1.1), Windows 98+ (WinSock2)
 * Compiler:  GCC, Clang, MSVC, MinGW
 *
 * For Windows 95 support, define PLAT_WIN95 before including this header
 * or pass -DPLAT_WIN95 to the compiler. This switches to WinSock 1.1.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

/* ── OS detection ─────────────────────────────────────────────────────── */
#if defined(_WIN32) || defined(__CYGWIN__)
#  define PLAT_WINDOWS 1
#elif defined(__APPLE__)
#  define PLAT_MACOS 1
#  define PLAT_POSIX 1
#else
#  define PLAT_LINUX  1
#  define PLAT_POSIX  1
#endif

/* ── Windows ─────────────────────────────────────────────────────────── */
#ifdef PLAT_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef _WIN32_WINNT
#    ifdef PLAT_WIN95
#      define _WIN32_WINNT 0x0400  /* Windows 95 */
#    else
#      define _WIN32_WINNT 0x0410  /* Windows 98 */
#    endif
#  endif
#  ifdef PLAT_WIN95
     /* WinSock 1.1 — available on Win95 without extra updates */
#    include <winsock.h>
#  else
     /* WinSock 2 — Win98+, NT4 SP4+ */
#    include <winsock2.h>
#  endif
#  include <windows.h>
#  include <stdlib.h>   /* _fullpath */
#  include <time.h>

typedef SOCKET plat_sock_t;
#  define PLAT_INVALID_SOCK   INVALID_SOCKET
#  define plat_sock_valid(s)  ((s) != INVALID_SOCKET)
#  define plat_sock_close(s)  closesocket(s)
#  ifndef PATH_MAX
#    define PATH_MAX          MAX_PATH
#  endif
#  define SIGPIPE_IGNORE()    /* no SIGPIPE on Windows */

static __inline char *plat_realpath(const char *p, char *out) {
    return _fullpath(out, p, PATH_MAX);
}
static __inline struct tm *plat_localtime(const time_t *t, struct tm *r) {
#ifdef PLAT_WIN95
    /* Win95: use localtime() — avoids __localtime32_s which has i686 code */
    struct tm *tmp = localtime(t);
    if (tmp) *r = *tmp;
    return tmp ? r : (struct tm *)0;
#else
    localtime_s(r, t); return r;
#endif
}

static __inline int plat_init(void) {
    WSADATA w;
#ifdef PLAT_WIN95
    return WSAStartup(MAKEWORD(1,1), &w) != 0 ? -1 : 0;
#else
    return WSAStartup(MAKEWORD(2,2), &w) != 0 ? -1 : 0;
#endif
}
static __inline void plat_cleanup(void) { WSACleanup(); }

/* ── POSIX (Linux + macOS) ────────────────────────────────────────────── */
#else
#  ifndef _XOPEN_SOURCE
#    define _XOPEN_SOURCE 700
#  endif
#  include <unistd.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <signal.h>
#  include <stdlib.h>
#  include <time.h>

typedef int plat_sock_t;
#  define PLAT_INVALID_SOCK  (-1)
#  define plat_sock_valid(s) ((s) >= 0)
#  define plat_sock_close(s) close(s)
#  define SIGPIPE_IGNORE()   signal(SIGPIPE, SIG_IGN)

static inline struct tm *plat_localtime(const time_t *t, struct tm *r) {
    return localtime_r(t, r);
}
static inline char *plat_realpath(const char *p, char *out) {
    return realpath(p, out);
}
static inline int  plat_init(void)    { return 0; }
static inline void plat_cleanup(void) {}
#endif

/* ── send/recv ────────────────────────────────────────────────────────── */
#ifdef PLAT_WINDOWS
#  define plat_send(s,b,n) send((s),(b),(int)(n), 0)
#  define plat_recv(s,b,n) recv((s),(b),(int)(n), 0)
#elif defined(MSG_NOSIGNAL)
#  define plat_send(s,b,n) send((s),(b),(n), MSG_NOSIGNAL)
#  define plat_recv(s,b,n) recv((s),(b),(n), 0)
#else  /* macOS: set SO_NOSIGPIPE on socket instead */
#  define plat_send(s,b,n) send((s),(b),(n), 0)
#  define plat_recv(s,b,n) recv((s),(b),(n), 0)
#endif

/* ── path separator ───────────────────────────────────────────────────── */
#ifdef PLAT_WINDOWS
#  define PATH_SEP '\\'
#else
#  define PATH_SEP '/'
#endif

/* ── is_path_sep: matches both / and \ ───────────────────────────────── */
#define is_path_sep(c) ((c) == '/' || (c) == '\\')

#endif /* PLATFORM_H */
