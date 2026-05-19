# Architecture Overview — HTTP RetroServer

## Purpose

HTTP RetroServer is a lightweight, self-contained HTTP static file server written in C99. Its primary goal is maximum portability: a single binary serves files over HTTP on platforms ranging from Windows 95 on an i486 CPU to modern Linux and macOS systems, with no runtime dependencies beyond the host OS.

## Design Constraints

These constraints were explicitly chosen and drive most architectural decisions:

| Constraint | Implication |
|---|---|
| No external libraries | Everything is implemented in standard C99 + OS APIs |
| Single self-contained binary | Icons, HTML templates, and the startup banner are embedded at compile time |
| Retro browser compatibility | HTML output targets IE 3.0, Netscape 4, and Lynx — no CSS classes, no JavaScript |
| Windows 95 support | Requires WinSock 1.1, i486 instruction set, and Win95-era APIs only |
| Zero runtime configuration required | Sane defaults; `config.txt` is optional |

## System Context

```
┌────────────────────┐         HTTP/TCP          ┌──────────────────────────┐
│  HTTP Client       │◄─────────────────────────►│   HTTP RetroServer       │
│  (Browser, Lynx,   │         port 8080         │   (retroserver binary)   │
│   curl, wget…)     │                           └────────────┬─────────────┘
└────────────────────┘                                        │ read()
                                                              ▼
                                                  ┌──────────────────────────┐
                                                  │   Host Filesystem        │
                                                  │   (served root directory)│
                                                  └──────────────────────────┘
```

> See **[diagrams/01-context.puml](diagrams/01-context.puml)** for the PlantUML C4 context diagram.

## Architectural Style

RetroServer follows a **layered architecture** with three horizontal layers:

```
┌──────────────────────────────────────────────────┐
│                    server/                       │  ← HTTP logic
│  start_stop · connection_thread · request_handler│
│  static_handler · icons_handler · http_parser    │
├──────────────────────────────────────────────────┤
│                    utils/                        │  ← Shared utilities
│       config_loader · server_utils · log         │
├──────────────────────────────────────────────────┤
│                   platform/                      │  ← OS abstraction
│  platform.h · fs · thread · win95_compat         │
└──────────────────────────────────────────────────┘
```

Dependencies flow strictly **downward**: `server/` depends on `utils/` and `platform/`; `platform/` depends on nothing above it.

## Quality Attributes

| Attribute | Priority | Approach |
|---|---|---|
| **Portability** | Critical | Compile-time platform abstraction layer; three separate toolchain targets |
| **Simplicity** | High | Single process, thread-per-connection, no event loop framework |
| **Security** | High | Path traversal protection via `realpath()`, rate limiting per IP, connection cap |
| **Self-containment** | High | Assets embedded as C arrays at build time via code generation scripts |
| **Low resource usage** | Medium | 2 MB stack per thread, 200 connection limit, 4 KB I/O chunks |
| **Retro UI compatibility** | Medium | Plain HTML tables, GIF icons, no JavaScript |

## Runtime Limits

| Parameter | Value | Defined in |
|---|---|---|
| Max concurrent connections | 200 | `start_stop.c:MAX_CONNECTIONS` |
| Rate limit window | 5 seconds | `start_stop.c:RATE_WINDOW` |
| Max requests per IP per window | 500 | `start_stop.c:RATE_LIMIT` |
| Max IP table entries | 1024 | `start_stop.c:MAX_IPS` |
| Thread stack size | 2 MB | `start_stop.c:THREAD_STACK_SIZE` |
| Max directory entries | 8192 | `static_handler.c:MAX_DIR_ENTRIES` |
| Max raw HTTP request size | 16 KB | `request_handler.c:RAW_REQUEST_SIZE` |
| Socket read/write timeout | 10 seconds | `connection_thread.c` |

## Supported Platforms

| Platform | Compiler | Socket API | Threading |
|---|---|---|---|
| Linux | GCC / Clang | BSD sockets (POSIX) | pthreads |
| macOS | Clang / GCC | BSD sockets (POSIX) | pthreads |
| Windows 95 | MinGW-w64 (cross) | WinSock 1.1 (`wsock32.dll`) | Win32 threads |
| Windows 98–11 | MinGW-w64 (cross) | WinSock 2 (`ws2_32.dll`) | Win32 threads |

## Related Documents

- [02-components.md](02-components.md) — Module breakdown and internal dependencies
- [03-request-lifecycle.md](03-request-lifecycle.md) — Full HTTP request flow
- [04-build-and-embedding.md](04-build-and-embedding.md) — Build system and asset embedding
- [05-decisions.md](05-decisions.md) — Architecture Decision Records

## Author

**Jonathan Pablo Toledo M.**  
[TheRetroCenter.com](https://www.theretrocenter.com)
