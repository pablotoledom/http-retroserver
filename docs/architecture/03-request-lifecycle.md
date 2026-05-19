# HTTP Request Lifecycle — HTTP RetroServer

## Overview

RetroServer uses a **thread-per-connection** model. The main thread runs a blocking `select` loop accepting TCP connections; each accepted connection is handed off to a newly spawned OS thread that handles the entire HTTP exchange and then exits.

> See **[diagrams/04-request-sequence.puml](diagrams/04-request-sequence.puml)** for the sequence diagram.  
> See **[diagrams/05-server-states.puml](diagrams/05-server-states.puml)** for the server state machine.

---

## Phase 1 — Server Startup

```
main()
  plat_init()              ← WinSock init on Windows; no-op on POSIX
  load_config()            ← reads configs/config.txt (optional)
  signal(SIGINT/SIGTERM)   ← both route to server_stop()
  server_start(root, port)
```

Inside `server_start` (`start_stop.c`):

1. `make_listen_socket(port)` — creates a TCP socket, sets `SO_REUSEADDR`, binds to `0.0.0.0:<port>`, calls `listen(128)`.
2. Initialises the connection mutex (`plat_mutex_init`).
3. Enters the **accept loop**.

---

## Phase 2 — Accept Loop

```
while (running) {
    select(server_fd)        ← blocks until a connection arrives or SIGINT
    accept()                 ← returns client socket
    too_many_connections()   ← per-IP rate limit check
    try_register_connection() ← global cap: max 200
    plat_thread_spawn(connection_thread, args)
}
```

### Rate Limiting

`too_many_connections(ip_addr)` maintains a flat array of up to 1024 IP entries (`ip_table`). Each entry records:

- `count` — connections in the current `RATE_WINDOW` (5 s)
- `blocked` — whether the IP is temporarily banned
- `last_conn` / `last_rejected` — timestamps for window reset

An IP is blocked when it exceeds **500 requests within any 5-second window**. It is automatically unblocked after `2 × RATE_WINDOW` (10 s) without a new connection.

---

## Phase 3 — Connection Thread (`connection_thread.c`)

Each spawned thread:

1. Receives `thread_args { client_socket, root_directory }`.
2. Allocates a `connection_ctx_t` wrapping the socket.
3. Sets `SO_RCVTIMEO` / `SO_SNDTIMEO` to **10 seconds** on the socket.
4. Calls `request_handler(plain_read, conn, root_directory)`.
5. After `request_handler` returns: `connection_close(conn)`, `free(conn)`, `free(args)`, `unregister_connection()`.
6. Thread exits.

The thread is **detached** — the main loop never joins it.

---

## Phase 4 — Request Parsing (`request_handler.c`)

`request_handler` reads raw bytes via `plain_read` into a 16 KB buffer, looping until it finds `\r\n\r\n` (end of HTTP headers).

`parse_http_request` (`http_request_parser.c`) extracts:
- `method` (e.g. `GET`)
- `url` (raw path + query string)
- `version` (e.g. `HTTP/1.1`)
- Up to 64 `headers[]` key/value pairs
- `body` pointer (if `Content-Length` is present)

**Validation:**
- Parse failure → `400 Bad Request`
- Method other than `GET` or `HEAD` → `405 Method Not Allowed` (with `Allow: GET, HEAD`)

---

## Phase 5 — Routing (`request_handler.c`)

```
url_decode(path)

if path starts with "/_icons/"
    → serve_icon(ctx, icon_name)        icons_handler.c

else
    extract ?dl → force_download flag
    → handle_static_file_or_directory(ctx, root, path, force_download)
```

---

## Phase 6 — Static Handler (`static_handler.c`)

### Security: Path Traversal Guard

Two checks are applied before any file is opened:

1. **`..` rejection** — if the decoded URL contains `..`, immediately return `403 Forbidden`.
2. **`realpath` confinement** — the candidate full path is resolved with `plat_realpath`. If the result does not start with `root_directory`, the request is blocked with `403 Forbidden`. This catches symlink escapes.

### File vs Directory

| Condition | Action |
|---|---|
| Path not found | `404 Not Found` |
| Path is a directory, no trailing `/` | `301 Moved Permanently` to `<path>/` |
| Path is a directory, has trailing `/` | `send_directory_listing()` |
| Path is a file | `serve_file()` |

### `serve_file()`

1. Opens the file (`open()` on POSIX, `CreateFile()` on Windows).
2. Calls `get_mime_type(path)` for the `Content-Type` header.
3. If `force_download`: overrides MIME to `application/octet-stream`, adds `Content-Disposition: attachment`.
4. Sends the HTTP header with `Content-Length`.
5. Streams the file in **4 KB chunks** (`CHUNK_SIZE`).

### `send_directory_listing()`

1. Opens the directory with `fs_opendir`.
2. Reads up to 8192 entries into a heap-allocated `DirEntry[]`.
3. Sorts: directories first, then alphabetically (`qsort`).
4. Sends the HTTP header (no `Content-Length` — browser reads until connection close).
5. Streams the HTML header template (`dir_header.html` with `{{PATH}}` substituted).
6. Emits one `<tr>` row per entry, inline-computed into a 2 KB stack buffer.
7. Streams the HTML footer template (`dir_footer.html`).

---

## Phase 7 — Icon Handler (`icons_handler.c`)

Receives a bare icon filename (e.g. `folder.gif`). Looks it up in the `icons_data` table (a C array embedded at compile time). Sends `Content-Type: image/gif` with the raw GIF bytes. Returns `404` if the name is not in the table.

---

## Error Responses

All error responses use `send_status(ctx, code, status)`:

1. Sends a minimal HTTP header (`Content-Type: text/html`, `Connection: close`).
2. Renders `error.html` (embedded) with `{{CODE}}` and `{{STATUS}}` substituted.
3. Sends the rendered body.

---

## Full Sequence (happy path — file download)

```
Client          start_stop      connection_thread   request_handler   static_handler   Filesystem
  │                │                   │                  │                 │               │
  │── TCP SYN ────►│                   │                  │                 │               │
  │                │ select() ready    │                  │                 │               │
  │                │ accept()          │                  │                 │               │
  │                │ rate_limit_ok?    │                  │                 │               │
  │                │ conn_limit_ok?    │                  │                 │               │
  │                │─── spawn ────────►│                  │                 │               │
  │                │                   │ set SO_RCVTIMEO  │                 │               │
  │── GET /file ──────────────────────►│                  │                 │               │
  │                │                   │── plain_read() ─►│                 │               │
  │                │                   │                  │ parse_http_request()            │
  │                │                   │                  │ url_decode()    │               │
  │                │                   │                  │─── handle_static ──────────────►│
  │                │                   │                  │                 │ check ".."    │
  │                │                   │                  │                 │ realpath()    │
  │                │                   │                  │                 │── open() ────►│
  │◄── HTTP 200 + body ─────────────────────────────────────────────────────│               │
  │                │                   │ connection_close()│                │               │
  │                │                   │ unregister_connection()            │               │
  │                │                   │ thread exits      │                │               │
```

## Related Documents

- [01-overview.md](01-overview.md) — Architecture overview
- [02-components.md](02-components.md) — Module breakdown and internal dependencies
- [04-build-and-embedding.md](04-build-and-embedding.md) — Build system and asset embedding
- [05-decisions.md](05-decisions.md) — Architecture Decision Records

## Author

**Jonathan Pablo Toledo M.**  
[TheRetroCenter.com](https://www.theretrocenter.com)
