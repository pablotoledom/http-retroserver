# HTTP RetroServer

> [Versión en Español](LEAME.md)

A lightweight HTTP static file server written in C. Designed for maximum retro compatibility — the directory listing works on browsers as old as Internet Explorer 3.0 and Lynx, with a phosphor green terminal aesthetic. Runs on Linux, macOS, and Windows 98+.

---

## Features

- Serves static files with an auto-generated directory listing
- Force-download any file with the `?dl` query parameter
- MIME type detection for common file formats
- Multi-threaded connection handling
- Path traversal protection (blocks `..` and symlink escapes)
- Configurable via a plain text config file
- Graceful shutdown on `SIGINT` / `SIGTERM`
- Installable as a systemd service (Linux)
- **Retro compatible:** tested down to IE 3.0, Netscape 4, and Lynx
- **Phosphor green UI:** black background, green terminal style
- **Self-contained binary:** icons and HTML templates are embedded at compile time — no extra files needed at runtime
- **Cross-platform:** Linux, macOS, Windows 98+ (no external dependencies)

## Requirements

- GCC, Clang, or MinGW
- CMake 3.10+

No external libraries required.

```bash
# Debian / Ubuntu / Raspberry Pi
sudo apt install build-essential cmake

# macOS
brew install cmake

# Windows — install MinGW-w64 and CMake
```

## Build

### Linux / macOS

```bash
# Production build
./retroserver.sh compile

# Debug build
./retroserver.sh compiledebug
```

### Windows (MinGW)

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

The binary is placed in `bin/retroserver` (or `bin/retroserver.exe` on Windows) along with `configs/`. Icons and HTML templates are compiled directly into the binary — no additional folders needed at runtime.

## Usage

```bash
./retroserver.sh run [directory]
```

If no directory is specified, the current directory is served.

**Examples:**

```bash
# Serve the current directory
./retroserver.sh run

# Serve a specific directory
./retroserver.sh run /home/user/files
```

Once running, open your browser at `http://localhost:8080`

## Configuration

Edit `configs/config.txt`:

```ini
# Log level: 0=none 1=error 2=warn 3=info 4=debug
verbose_level=3

# HTTP port
http_port=8080
```

## Customizing the UI

### HTML Templates

The page structure lives in `html/`. Edit these files and recompile to apply changes:

| File | Description | Variables |
|------|-------------|-----------|
| `html/dir_header.html` | Page header and listing opening | `{{PATH}}` |
| `html/dir_footer.html` | Page footer | *(none)* |
| `html/error.html` | Error page | `{{CODE}}`, `{{STATUS}}` |

At compile time, these files are embedded into the binary automatically.

### Icons

Icons are 16×16 GIF images in `icons/`. To customize:

1. Replace any `.gif` in `icons/` with your own image
2. Recompile — CMake detects the change and re-embeds everything

| Icon | File types |
|------|------------|
| `[FOLDER]` | Directories |
| `[MUSIC]` | mp3, ogg, wav, flac, aac, m4a, wma |
| `[VIDEO]` | mp4, mkv, avi, mov, webm, flv, wmv |
| `[IMAGE]` | jpg, jpeg, png, gif, svg, webp, ico, bmp |
| `[ZIPED]` | zip, gz, tar, 7z, rar, bz2, xz |
| `[PDF]` | pdf |
| `[DISC]` | iso, cue |
| `[PROG]` | exe, bin |
| `[TEXT]` | txt, md, log, csv, rtf |
| `[CODE]` | c, h, cpp, py, sh, js, json, html, xml, go, rs, yaml |
| `[FILE]` | Everything else |

> In text browsers like Lynx, these labels are shown in place of the icons, padded to a fixed width for column alignment.

## Systemd Service (Linux)

```bash
# Install as a service
sudo ./retroserver.sh install

# Remove the service
sudo ./retroserver.sh uninstall
```

## Available Commands

| Command        | Description                      |
|----------------|----------------------------------|
| `compile`      | Build for production (Release)   |
| `compiledebug` | Build with debug symbols         |
| `run [dir]`    | Run the server (default: `.`)    |
| `install`      | Install as a systemd service     |
| `uninstall`    | Remove the systemd service       |

## Cross-Platform Notes

| Platform | Compiler | Min version |
|----------|----------|-------------|
| Linux | GCC / Clang | Any modern |
| macOS | Clang / GCC | 10.9+ |
| Windows | MinGW-w64 | Windows 98 (WinSock2 required) |

Windows 95 is supported if the WinSock2 update is installed. Windows 98 and later include it by default.

## Project Structure

```
retroserver/
├── src/
│   ├── main.c
│   ├── platform/                     # Cross-platform abstraction layer
│   │   ├── platform.h                # OS detection, socket types, inline helpers
│   │   ├── fs.h                      # Filesystem API
│   │   ├── fs_posix.c                # opendir/stat (Linux + macOS)
│   │   ├── fs_win32.c                # FindFirstFile (Windows)
│   │   ├── thread.h                  # Threading/mutex API
│   │   ├── thread_posix.c            # pthreads
│   │   └── thread_win32.c            # CreateThread + CRITICAL_SECTION
│   ├── server/
│   │   ├── start_stop.c              # Server lifecycle
│   │   ├── connection.c              # Socket read/write abstraction
│   │   ├── connection_thread.c       # Thread-per-connection handler
│   │   ├── http_request_parser.c
│   │   ├── request_handler.c         # Routes requests
│   │   ├── static_handler.c          # File serving and directory listing
│   │   └── icons_handler.c           # Serves embedded icons at /_icons/
│   └── utils/
│       ├── config_loader.c
│       ├── server_utils.c            # MIME types, URL encode/decode, template engine
│       └── log.h
├── html/                             # HTML templates (embedded at compile time)
│   ├── dir_header.html
│   ├── dir_footer.html
│   └── error.html
├── icons/                            # GIF icons 16×16 (embedded at compile time)
├── configs/
│   └── config.txt
├── scripts/
│   ├── gen_icons_c.sh                # Embeds icons into C source (run by CMake)
│   ├── gen_html_c.sh                 # Embeds HTML into C source (run by CMake)
│   └── ...
└── retroserver.sh                    # Main entry point (Linux/macOS)
```

---

## Author

**Jonathan P. Toledo**  
[TheRetroCenter.com](https://www.theretrocenter.com)
