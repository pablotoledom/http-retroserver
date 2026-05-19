# Build System and Asset Embedding — HTTP RetroServer

## Overview

The build system is CMake 3.10+, orchestrated through the `retroserver.sh` shell script. Its central responsibility beyond compiling C sources is **embedding binary and text assets directly into the executable** at compile time, so the final binary has zero runtime file dependencies.

> See **[diagrams/06-build-pipeline.puml](diagrams/06-build-pipeline.puml)** for the full build activity diagram.  
> See **[diagrams/07-platform-abstraction.puml](diagrams/07-platform-abstraction.puml)** for the platform abstraction class diagram.

---

## Entry Point — `retroserver.sh`

The script wraps CMake commands behind simple verbs:

| Command | CMake invocation | Output directory |
|---|---|---|
| `linux` | Native GCC, Release | `bin-linux/` |
| `macos` | Native Clang, Release | `bin-macos/` |
| `win32` | MinGW-w64 cross toolchain | `bin-win32/` |
| `debug` | Native GCC, Debug symbols | `bin-linux/` |
| `run [dir]` | Runs the existing binary | — |
| `install` | Copies binary + systemd unit | `/usr/local/bin/` |
| `uninstall` | Removes installed files | — |

---

## Asset Embedding Pipeline

Three CMake `add_custom_command` targets run **before** compilation. Each generates a `.c` / `.h` pair that is included in the main `add_executable` source list.

### 1. Icon Embedding (`gen_icons_c.sh`)

```
icons/*.gif  ──►  gen_icons_c.sh  ──►  icons_data.c / icons_data.h
```

- Reads every `.gif` in `icons/`.
- Encodes each file as a `static const unsigned char icon_<name>[]` array.
- Generates a lookup table: `{ filename, data_ptr, size }`.
- `icons_handler.c` calls `icon_lookup(name)` to find and serve any icon by name.
- CMake dependency: re-runs whenever any `.gif` changes (`DEPENDS ${ICON_GIF_FILES}`).

### 2. HTML Template Embedding (`gen_html_c.sh`)

```
html/*.html  ──►  gen_html_c.sh  ──►  html_data.c / html_data.h
```

- Reads `dir_header.html`, `dir_footer.html`, and `error.html`.
- Stores each as a null-terminated C string.
- Exposes `html_get(name, &len)` for lookup by filename.
- `static_handler.c` calls `html_get("dir_header.html", &len)` then runs `tmpl_append` to substitute `{{PATH}}`, `{{CODE}}`, `{{STATUS}}`.

### 3. Banner Embedding (`gen_banner_c.sh`)

```
scripts/show/welcome  ──►  gen_banner_c.sh  ──►  banner_data.c / banner_data.h
```

- Reads the ASCII art welcome file.
- Stores it as `const char BANNER[]`.
- `main()` calls `printf("%s", BANNER)` at startup.

---

## Platform-Specific Compilation

CMake selects source files based on the target OS:

```cmake
if(WIN32)
    PLATFORM_SOURCES = fs_win32.c + thread_win32.c [+ win95_compat.c]
else()
    PLATFORM_SOURCES = fs_posix.c + thread_posix.c
```

### Toolchain Files (`cmake/`)

| File | Target | Key flags |
|---|---|---|
| `mingw-w64-64.cmake` | Windows 98–11 (64-bit) | `x86_64-w64-mingw32-gcc`, links `ws2_32` statically |
| `mingw-w64-32.cmake` | Windows 98–11 (32-bit) | `i686-w64-mingw32-gcc`, links `ws2_32` statically |
| `mingw-w64-win95.cmake` | Windows 95 (32-bit, i486) | `i486-w64-mingw32-gcc`, defines `PLAT_WIN95`, links `wsock32` dynamically |

### Linking Strategy

| Target | Strategy | Reason |
|---|---|---|
| Linux / macOS | Dynamic (system libc + libpthread) | Standard; libc always present |
| Windows 98–11 | Static (`-static`) | Avoids MSVCRT DLL version conflicts |
| Windows 95 | Dynamic against system DLLs only | `msvcrt.dll`, `wsock32.dll`, `kernel32.dll` are guaranteed on Win95; static CRT has i686 code |

---

## The i486 Patch (`scripts/patch_win32.c`)

MinGW's optimizer emits `CMOVcc` instructions (conditional move, introduced in i686/Pentium Pro). The Windows 95 build targets the i486 instruction set — CPUs that predate the Pentium Pro and cannot execute `CMOVcc`.

After linking, `patch_win32.c` is compiled and run as a post-processing step:

1. Opens the `.exe` as a binary file.
2. Scans for `CMOVcc` opcode sequences (two-byte prefix `0F 4x`).
3. Replaces each with an equivalent `Jcc` + `MOV` sequence that runs on i486.

This allows the binary to use standard MinGW toolchains without requiring a custom compiler that targets i486 natively.

---

## Build Output Structure

After a successful build, each target directory is self-contained:

```
bin-linux/
├── retroserver          ← static binary; icons, HTML, banner embedded
└── configs/
    └── config.txt       ← only runtime dependency (optional)

bin-win32/
├── retroserver.exe      ← patched i486-compatible PE binary
└── configs/
    └── config.txt
```

No `icons/` or `html/` directories are needed at runtime.

---

## Systemd Integration (Linux)

`scripts/retroserver.service` is a standard systemd unit file. `retroserver.sh install` copies:

- The binary to `/usr/local/bin/retroserver`
- The unit file to `/etc/systemd/system/retroserver.service`
- Calls `systemctl daemon-reload && systemctl enable retroserver`

`retroserver.sh uninstall` reverses these steps.
