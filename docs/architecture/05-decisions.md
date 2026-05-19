# Architecture Decision Records — HTTP RetroServer

Each record follows the format: **Context → Decision → Consequences**.

---

## ADR-001 — No External Libraries

**Status:** Accepted

**Context:**  
The server needs to run on Windows 95, Linux, and macOS. Most HTTP or networking libraries either require a modern OS, have their own build systems, or introduce DLL/shared-object dependencies that complicate distribution.

**Decision:**  
Use only the C standard library and OS-provided APIs (POSIX sockets on Linux/macOS, WinSock on Windows). Implement all required functionality — HTTP parsing, MIME detection, URL encoding, template substitution — in-house.

**Consequences:**
- (+) The binary ships with zero runtime library dependencies.
- (+) The project compiles with any C99-capable compiler and CMake.
- (+) The entire codebase fits in a single directory without submodules or package managers.
- (–) HTTP parsing, MIME tables, and URL codec had to be written from scratch and are not battle-hardened against adversarial input in the way that a library like `llhttp` would be.
- (–) Only GET and HEAD methods are supported (no POST, no WebSocket, no TLS).

---

## ADR-002 — Thread-per-Connection Model

**Status:** Accepted

**Context:**  
Two common concurrency models for a TCP server are: (a) **thread-per-connection** — one OS thread per active socket; (b) **event loop / async I/O** — a single thread or a fixed pool multiplexes sockets via `epoll`/`kqueue`/`IOCP`.

**Decision:**  
Use thread-per-connection. The main thread runs a `select`-based accept loop and calls `plat_thread_spawn` for each accepted connection.

**Consequences:**
- (+) Code is straightforward sequential I/O per thread — no callbacks, no coroutines, no state machines inside the handler.
- (+) Works on Windows 95 (`CreateThread`) and POSIX (`pthread_create`) with the same abstraction.
- (+) A slow or blocked connection cannot stall the accept loop or other connections.
- (–) Each thread allocates a 2 MB stack, capping practical concurrency to ~200 connections before memory pressure becomes significant (hence `MAX_CONNECTIONS = 200`).
- (–) An event-driven model would scale to tens of thousands of concurrent connections; this server is designed for LAN/local use where that scale is irrelevant.

---

## ADR-003 — C99 as the Language Standard

**Status:** Accepted

**Context:**  
The Windows 95 cross-compile uses MinGW-w64 targeting an i486 CPU. Older MinGW versions bundled with some Linux distributions have incomplete C11 support. C89 would be unnecessarily restrictive (no `//` comments, no `<stdbool.h>`, no mixed declarations and code). C11 and later introduce features not reliably available on all target toolchains.

**Decision:**  
Target C99 (`set(CMAKE_C_STANDARD 99)` in CMakeLists.txt).

**Consequences:**
- (+) `//` comments, `<stdint.h>`, `<stdbool.h>`, variable-length array declarations, designated initializers, and `snprintf` are all available.
- (+) Supported by every GCC and Clang version relevant to any supported platform.
- (–) C11 atomics (`_Atomic`) are unavailable; the `volatile int running` flag in `start_stop.c` is used instead (sufficient for the signal-handler use case).

---

## ADR-004 — Compile-Time Asset Embedding

**Status:** Accepted

**Context:**  
The server needs HTML templates, GIF icons, and a startup banner. These could be loaded from the filesystem at runtime (requiring the operator to keep supporting files next to the binary), or embedded into the binary.

**Decision:**  
Embed all assets as C byte arrays at compile time using three code generation shell scripts invoked by CMake custom commands. The scripts convert binary/text files into `.c`/`.h` pairs that are compiled directly into the executable.

**Consequences:**
- (+) The deployed binary is fully self-contained — copy the executable to any machine and run it.
- (+) No "file not found at runtime" class of errors for assets.
- (+) Icons and templates can be customized by editing source files and recompiling; the change is guaranteed to be in sync with the binary.
- (–) Changing an icon or HTML template requires a recompile; there is no hot-reload.
- (–) The `configs/config.txt` file is intentionally **not** embedded, because it is the one thing operators are expected to change without recompiling.

---

## ADR-005 — WinSock 1.1 for Windows 95 Support

**Status:** Accepted

**Context:**  
Windows 95 ships with WinSock 1.1 (`wsock32.dll`). WinSock 2 (`ws2_32.dll`) was introduced with Windows 98 and NT 4 SP4. Using WinSock 2 APIs would exclude Windows 95.

**Decision:**  
When `PLAT_WIN95` is defined (set by the `mingw-w64-win95.cmake` toolchain), `platform.h` includes `<winsock.h>` (1.1) instead of `<winsock2.h>`, and `WSAStartup` is called with `MAKEWORD(1,1)`. The server socket is created with `AF_INET / SOCK_STREAM`, which is identical in both versions.

**Consequences:**
- (+) The same binary runs on Windows 95, 98, NT, XP, and Windows 11 — a range of 30+ years of Windows versions.
- (–) WinSock 1.1 lacks some convenience functions available in WinSock 2 (e.g., `WSAAddressToString`). These are not needed for a basic TCP server.
- (–) `wsock32.dll` is linked dynamically (not statically) because the Win95 static CRT contains i686 instructions. This means `wsock32.dll` must be present on the target — it always is on any genuine Windows 95+ installation.

---

## ADR-006 — i486 Instruction Set Target with Post-Link Patching

**Status:** Accepted

**Context:**  
The Windows 95 build must run on any x86 CPU from the 486 onward, including the original Pentium (P5), which does not support `CMOVcc` instructions. MinGW's optimizer emits `CMOVcc` even when targeting `i486` because the GCC backend treats it as an optimization hint rather than a hard ABI constraint.

**Decision:**  
After linking, run `scripts/patch_win32.c` as a binary post-processor to scan the PE executable and rewrite any `CMOVcc` opcodes (`0F 4x`) into semantically equivalent `Jcc` + `MOV` sequences that are valid on i486.

**Consequences:**
- (+) The binary runs on 486 and Pentium CPUs without requiring a special i486-strict GCC build.
- (+) The standard MinGW-w64 toolchain available in most Linux package managers can be used.
- (–) The patch is a brittle binary rewriter; if the compiler ever emits `CMOVcc` in a context the patcher does not recognize, the patched binary could be incorrect.
- (–) Adds a post-link step that must be kept in sync with the compiler's output format.

## Related Documents

- [01-overview.md](01-overview.md) — Architecture overview
- [02-components.md](02-components.md) — Module breakdown and internal dependencies
- [03-request-lifecycle.md](03-request-lifecycle.md) — Full HTTP request flow
- [04-build-and-embedding.md](04-build-and-embedding.md) — Build system and asset embedding

## Author

**Jonathan Pablo Toledo M.**  
[TheRetroCenter.com](https://www.theretrocenter.com)
