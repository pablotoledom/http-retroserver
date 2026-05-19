# HTTP RetroServer

> [English Version](README.md)

Servidor HTTP de archivos estáticos ligero, escrito en C. Diseñado para máxima compatibilidad retro — el listado de directorios funciona en navegadores tan antiguos como Internet Explorer 3.0 y Lynx, con una estética de terminal fósforo verde. Corre en Linux, macOS y Windows 95 hasta Windows 11.


![alt Running on Windows 95](https://raw.githubusercontent.com/pablotoledom/http-retroserver/main/docs/images/condensed-example.jpg)

---

## Características

- Sirve archivos estáticos con un listado de directorios autogenerado
- Descarga forzada de cualquier archivo con el parámetro `?dl`
- Detección de tipo MIME para formatos de archivo comunes
- Manejo de conexiones multi-hilo
- Protección contra path traversal (bloquea `..` y escapes por symlink)
- Configurable mediante un archivo de texto plano
- Apagado graceful con `SIGINT` / `SIGTERM`
- Instalable como servicio systemd (Linux)
- **Retro compatible:** probado hasta IE 3.0, Netscape 4 y Lynx
- **UI fósforo verde:** fondo negro, estilo terminal verde
- **Binario autocontenido:** iconos, templates HTML y banner de inicio se embeben en tiempo de compilación — no se necesitan archivos adicionales en producción
- **Multiplataforma:** Linux, macOS, Windows 95+ (sin dependencias externas)

## Requisitos

- GCC o Clang
- CMake 3.10+

No se requieren librerías externas.

```bash
# Debian / Ubuntu / Raspberry Pi
sudo apt install build-essential cmake

# macOS
brew install cmake

# Cross-compilar para Windows desde Linux
sudo apt install build-essential cmake mingw-w64
```

## Compilación

```bash
# Linux  ->  bin-linux/retroserver
./retroserver.sh linux

# macOS  ->  bin-macos/retroserver   (ejecutar en una Mac)
./retroserver.sh macos

# Windows 95 a Win11 (32-bit)  ->  bin-win32/retroserver.exe
./retroserver.sh win32

# Linux con simbolos de debug
./retroserver.sh debug
```

Cada plataforma genera su propio directorio de salida junto con `configs/`.
Los iconos, templates HTML y el banner de inicio se compilan directamente
dentro del binario — no se necesitan carpetas adicionales en runtime.

## Uso

```bash
./retroserver.sh run [directorio]
```

Si no se especifica directorio, se sirve el directorio actual.

**Ejemplos:**

```bash
# Servir el directorio actual
./retroserver.sh run

# Servir un directorio específico
./retroserver.sh run /home/usuario/archivos
```

Una vez iniciado, abrir el navegador en `http://localhost:8080`

## Configuración

Editar `configs/config.txt`:

```ini
# Nivel de log: 0=ninguno 1=error 2=warn 3=info 4=debug
verbose_level=1

# Puerto HTTP
http_port=8080
```

## Personalizar la UI

### Banner de inicio

El arte ASCII que se muestra al iniciar proviene de `scripts/show/welcome`.
Editar ese archivo y recompilar — el banner se embebe automáticamente en el
binario mediante `scripts/gen_banner_c.sh` durante la compilación.

### Templates HTML

La estructura de las páginas vive en `html/`. Editar estos archivos y recompilar para aplicar cambios:

| Archivo | Descripción | Variables |
|---------|-------------|-----------|
| `html/dir_header.html` | Cabecera de página y apertura del listado | `{{PATH}}` |
| `html/dir_footer.html` | Pie de página | *(ninguna)* |
| `html/error.html` | Página de error | `{{CODE}}`, `{{STATUS}}` |

En tiempo de compilación, estos archivos se embeben automáticamente en el binario.

### Iconos

Los iconos son imágenes GIF de 16x16 en `icons/`. Para personalizar:

1. Reemplazar cualquier `.gif` en `icons/` por una imagen propia
2. Recompilar — CMake detecta el cambio y re-embebe todo

| Icono | Tipos de archivo |
|-------|-----------------|
| `[FOLDER]` | Directorios |
| `[MUSIC]` | mp3, ogg, wav, flac, aac, m4a, wma |
| `[VIDEO]` | mp4, mkv, avi, mov, webm, flv, wmv |
| `[IMAGE]` | jpg, jpeg, png, gif, svg, webp, ico, bmp |
| `[ZIPED]` | zip, gz, tar, 7z, rar, bz2, xz |
| `[PDF]` | pdf |
| `[DISC]` | iso, cue |
| `[PROG]` | exe, bin |
| `[TEXT]` | txt, md, log, csv, rtf |
| `[CODE]` | c, h, cpp, py, sh, js, json, html, xml, go, rs, yaml |
| `[FILE]` | Todo lo demas |

> En navegadores de texto como Lynx, estas etiquetas se muestran en lugar
> de los iconos, con padding fijo para alinear las columnas.

## Servicio Systemd (Linux)

```bash
# Instalar como servicio
sudo ./retroserver.sh install

# Eliminar el servicio
sudo ./retroserver.sh uninstall
```

## Comandos Disponibles

| Comando      | Descripcion                                    | Destino            |
|--------------|------------------------------------------------|--------------------|
| `linux`      | Compilar para Linux (Release)                  | `bin-linux/`       |
| `macos`      | Compilar para macOS (ejecutar en Mac)          | `bin-macos/`       |
| `win32`      | Cross-compilar para Windows 95 a Win11         | `bin-win32/`       |
| `debug`      | Linux con simbolos de debug                    | `bin-linux/`       |
| `run [dir]`  | Iniciar el servidor (por defecto: directorio actual) |               |
| `install`    | Instalar como servicio systemd                 |                    |
| `uninstall`  | Eliminar el servicio systemd                   |                    |

## Compatibilidad por Plataforma

| Plataforma | Compilador | Versiones compatibles |
|------------|------------|-----------------------|
| Linux | GCC / Clang | Cualquier distribucion moderna |
| macOS | Clang / GCC | 10.9+ |
| Windows | MinGW-w64 (cross desde Linux) | Windows 95 a Windows 11 |

El build de Windows usa WinSock 1.1 (`wsock32.dll`), presente en todas las
versiones de Windows desde 95 sin necesidad de actualizaciones adicionales.
El binario apunta al conjunto de instrucciones i486, por lo que funciona en
cualquier CPU x86 desde el 486 hasta los procesadores modernos.

## Estructura del Proyecto

```
retroserver/
├── src/
│   ├── main.c
│   ├── platform/                     # Capa de abstraccion multiplataforma
│   │   ├── platform.h                # Deteccion de OS, tipos de socket, helpers
│   │   ├── fs.h / fs_posix.c         # API de filesystem (Linux + macOS)
│   │   ├── fs_win32.c                # API de filesystem (Windows)
│   │   ├── thread.h / thread_posix.c # API de threads (pthreads)
│   │   ├── thread_win32.c            # API de threads (Win32)
│   │   └── win95_compat.c            # Overrides de inicio para Win95 (i486-safe)
│   ├── server/
│   │   ├── start_stop.c              # Ciclo de vida del servidor
│   │   ├── connection.c              # Abstraccion de lectura/escritura de socket
│   │   ├── connection_thread.c       # Manejador de conexiones por hilo
│   │   ├── http_request_parser.c
│   │   ├── request_handler.c         # Enrutamiento de peticiones
│   │   ├── static_handler.c          # Servido de archivos y listado de directorios
│   │   └── icons_handler.c           # Sirve los iconos embebidos en /_icons/
│   └── utils/
│       ├── config_loader.c
│       ├── server_utils.c            # Tipos MIME, URL encode/decode, motor de templates
│       └── log.h
├── html/                             # Templates HTML (embebidos en tiempo de compilacion)
│   ├── dir_header.html
│   ├── dir_footer.html
│   └── error.html
├── icons/                            # Iconos GIF 16x16 (embebidos en tiempo de compilacion)
├── cmake/
│   └── mingw-w64-win95.cmake         # Toolchain para cross-compilacion Windows
├── configs/
│   └── config.txt
├── scripts/
│   ├── compile_linux.sh
│   ├── compile_macos.sh
│   ├── compile_win32.sh
│   ├── compile_debug.sh
│   ├── patch_win32.c                 # Reemplaza instrucciones CMOVcc (i686) por i486
│   ├── gen_icons_c.sh                # Embebe iconos en C (ejecutado por CMake)
│   ├── gen_html_c.sh                 # Embebe HTML en C (ejecutado por CMake)
│   ├── gen_banner_c.sh               # Embebe banner de inicio en C (ejecutado por CMake)
│   └── show/
│       └── welcome                   # Fuente del arte ASCII del banner
└── retroserver.sh                    # Punto de entrada principal
```

---

## Autor

**Jonathan Pablo Toledo M.**  
[TheRetroCenter.com](https://www.theretrocenter.com)
