# Component Architecture — HTTP RetroServer

## Module Map

The source tree is divided into three layers plus a set of compile-time generated files:

```
src/
├── main.c                        Entry point — wires everything together
├── platform/                     OS abstraction layer
│   ├── platform.h                Socket types, plat_init/cleanup, path helpers
│   ├── fs.h                      Filesystem API declaration
│   ├── fs_posix.c                Filesystem impl — Linux + macOS
│   ├── fs_win32.c                Filesystem impl — Windows 95–11
│   ├── thread.h                  Thread/mutex API declaration
│   ├── thread_posix.c            Thread impl — pthreads
│   ├── thread_win32.c            Thread impl — Win32 threads
│   └── win95_compat.c            Win95 startup overrides (i486-safe entry point)
├── server/                       HTTP server logic
│   ├── start_stop.c/h            Server lifecycle, accept loop, rate limiting
│   ├── connection.c/h            Socket read/write abstraction (connection_ctx_t)
│   ├── connection_thread.c/h     Thread entry: owns one connection end-to-end
│   ├── http_request_parser.c/h   Parses raw HTTP/1.x request headers
│   ├── request_handler.c/h       Routes request to the correct handler
│   ├── static_handler.c/h        File serving and directory listing
│   └── icons_handler.c/h         Serves embedded GIF icons at /_icons/
└── utils/                        Shared utilities
    ├── config_loader.c/h         Reads configs/config.txt into globals
    ├── server_utils.c/h          MIME types, URL encode/decode, template engine
    └── log.h                     LOG_INFO/WARN/ERROR macros (uses global log_level)

(generated at build time — not in the source tree)
├── icons_data.c/h                All GIF icons as C byte arrays
├── html_data.c/h                 dir_header.html, dir_footer.html, error.html as C strings
└── banner_data.c/h               ASCII art startup banner as a C string
```

> See **[diagrams/02-components-layers.puml](diagrams/02-components-layers.puml)** for the layered package diagram.  
> See **[diagrams/03-module-dependencies.puml](diagrams/03-module-dependencies.puml)** for the include-level dependency graph.

---

## Component Responsibilities

### `main.c`
Orchestrates startup: initializes the platform (`plat_init`), prints the banner, validates and resolves the root directory argument, loads the config, registers signal handlers, and calls `server_start`. On exit calls `plat_cleanup`.

### `platform/` — OS Abstraction Layer

| File | Responsibility |
|---|---|
| `platform.h` | Defines `plat_sock_t`, `PLAT_INVALID_SOCK`, `plat_send/recv`, `SIGPIPE_IGNORE`, `plat_init/cleanup`, `plat_realpath`, `plat_localtime`. Switches between POSIX and Win32 types at compile time. |
| `fs.h` + `fs_posix.c` | `fs_opendir / fs_readdir / fs_closedir` — POSIX `opendir/readdir` wrapped into `fs_dir_t / fs_entry_t` structs. |
| `fs_win32.c` | Same interface using `FindFirstFile / FindNextFile`. Handles the `..` exclusion that Win95 `FindFirstFile` does not filter automatically. |
| `thread.h` + `thread_posix.c` | `plat_thread_spawn` (creates a detached pthread), `plat_mutex_init/lock/unlock`. |
| `thread_win32.c` | Same interface using `CreateThread` and `CRITICAL_SECTION`. |
| `win95_compat.c` | Overrides `mainCRTStartup` to avoid i686 code paths in the MSVC CRT that MinGW links by default. Necessary for the i486 target. |

### `server/` — HTTP Server Logic

| File | Responsibility |
|---|---|
| `start_stop.c` | `server_start`: creates the listen socket, runs the `select`-based accept loop, enforces per-IP rate limiting and global connection cap (200), spawns one thread per accepted connection. `server_stop`: sets `running=0` and closes the server socket. |
| `connection.c` | Wraps a `plat_sock_t` into `connection_ctx_t`. Provides `plain_read` and `connection_write` (chunked `send` loop), and `connection_close`. |
| `connection_thread.c` | Thread entry point. Sets 10 s socket timeouts, calls `request_handler(plain_read, conn, root)`, then closes the connection and decrements the active count. |
| `http_request_parser.c` | Parses the raw request buffer into an `HttpRequest` struct (method, URL, HTTP version, headers array, body pointer). Supports up to 64 headers. |
| `request_handler.c` | Validates method (GET/HEAD only → 405 otherwise). URL-decodes the path, detects the `?dl` query param, routes `/_icons/*` to `icons_handler`, everything else to `static_handler`. |
| `static_handler.c` | Path traversal guard (`..` rejection + `realpath` confinement). Serves files with correct `Content-Type` and `Content-Length`. Generates streaming directory listings using embedded HTML templates. Issues `301` redirects for directories missing a trailing `/`. |
| `icons_handler.c` | Looks up the requested icon name in `icons_data` (the embedded GIF table) and sends the raw bytes with `image/gif` content type. |

### `utils/` — Shared Utilities

| File | Responsibility |
|---|---|
| `config_loader.c` | Reads `configs/config.txt` line by line. Sets global `http_port` and `verbose_level`. Silently skipped if the file is absent. |
| `server_utils.c` | `get_mime_type` (extension lookup table), `url_encode / url_decode`, `html_encode`, `tmpl_append` (simple `{{KEY}}` template substitution engine). |
| `log.h` | `LOG_INFO / LOG_WARN / LOG_ERROR / LOG_DEBUG` macros gated on the global `log_level`. Output goes to `stdout`/`stderr`. |

### Generated Assets

| File | Generator | Contents |
|---|---|---|
| `icons_data.c/h` | `scripts/gen_icons_c.sh` | Each `.gif` in `icons/` as a `static const unsigned char[]` + a lookup table by filename. |
| `html_data.c/h` | `scripts/gen_html_c.sh` | `dir_header.html`, `dir_footer.html`, `error.html` as null-terminated C strings. `html_get(name)` returns a pointer. |
| `banner_data.c/h` | `scripts/gen_banner_c.sh` | `scripts/show/welcome` file content as a single `BANNER` C string constant. |

---

## Dependency Graph (simplified)

```
main.c
  ├── platform/platform.h      (OS primitives)
  ├── utils/config_loader.h    (global config)
  └── server/start_stop.h
        ├── platform/thread.h
        └── server/connection_thread.h
              ├── server/connection.h
              └── server/request_handler.h
                    ├── server/http_request_parser.h
                    ├── server/static_handler.h
                    │     ├── platform/fs.h
                    │     ├── html_data.h          (generated)
                    │     └── utils/server_utils.h
                    └── server/icons_handler.h
                          └── icons_data.h         (generated)
```

All layers depend on `utils/log.h` (header-only macros, no link dependency).
