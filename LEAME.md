# HTTP RetroServer

> [English Version](README.md)

Servidor HTTP de archivos estáticos ligero, escrito en C. Diseñado para máxima compatibilidad retro — el listado de directorios funciona en navegadores tan antiguos como Internet Explorer 3.0 y Lynx, con una estética de terminal fósforo verde. Corre en Linux, macOS y Windows 98+.

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
- **Binario autocontenido:** iconos y templates HTML se embeben en tiempo de compilación — no se necesitan archivos adicionales en producción
- **Multiplataforma:** Linux, macOS, Windows 98+ (sin dependencias externas)

## Requisitos

- GCC, Clang o MinGW
- CMake 3.10+

No se requieren librerías externas.

```bash
# Debian / Ubuntu / Raspberry Pi
sudo apt install build-essential cmake

# macOS
brew install cmake

# Windows — instalar MinGW-w64 y CMake
```

## Compilación

### Linux / macOS

```bash
# Build de producción
./retroserver.sh compile

# Build de debug
./retroserver.sh compiledebug
```

### Windows (MinGW)

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

El binario queda en `bin/retroserver` (o `bin/retroserver.exe` en Windows) junto con `configs/`. Los iconos y templates HTML se compilan directamente dentro del binario — no se necesitan carpetas adicionales en runtime.

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
verbose_level=3

# Puerto HTTP
http_port=8080
```

## Personalizar la UI

### Templates HTML

La estructura de las páginas vive en `html/`. Editar estos archivos y recompilar para aplicar cambios:

| Archivo | Descripción | Variables |
|---------|-------------|-----------|
| `html/dir_header.html` | Cabecera de página y apertura del listado | `{{PATH}}` |
| `html/dir_footer.html` | Pie de página | *(ninguna)* |
| `html/error.html` | Página de error | `{{CODE}}`, `{{STATUS}}` |

En tiempo de compilación, estos archivos se embeben automáticamente en el binario.

### Iconos

Los iconos son imágenes GIF de 16×16 en `icons/`. Para personalizar:

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
| `[FILE]` | Todo lo demás |

> En navegadores de texto como Lynx, estas etiquetas se muestran en lugar de los iconos, con padding fijo para alinear las columnas.

## Servicio Systemd (Linux)

```bash
# Instalar como servicio
sudo ./retroserver.sh install

# Eliminar el servicio
sudo ./retroserver.sh uninstall
```

## Comandos Disponibles

| Comando        | Descripción                                  |
|----------------|----------------------------------------------|
| `compile`      | Compilar para producción (Release)           |
| `compiledebug` | Compilar con símbolos de debug               |
| `run [dir]`    | Iniciar el servidor (por defecto: `.`)       |
| `install`      | Instalar como servicio systemd               |
| `uninstall`    | Eliminar el servicio systemd                 |

## Notas de Compatibilidad por Plataforma

| Plataforma | Compilador | Versión mínima |
|------------|------------|----------------|
| Linux | GCC / Clang | Cualquier moderna |
| macOS | Clang / GCC | 10.9+ |
| Windows | MinGW-w64 | Windows 98 (requiere WinSock2) |

Windows 95 es compatible si se instala la actualización de WinSock2. Windows 98 y versiones posteriores la incluyen por defecto.

## Estructura del Proyecto

```
retroserver/
├── src/
│   ├── main.c
│   ├── platform/                     # Capa de abstracción multiplataforma
│   │   ├── platform.h                # Detección de OS, tipos de socket, helpers inline
│   │   ├── fs.h                      # API de filesystem
│   │   ├── fs_posix.c                # opendir/stat (Linux + macOS)
│   │   ├── fs_win32.c                # FindFirstFile (Windows)
│   │   ├── thread.h                  # API de threads/mutex
│   │   ├── thread_posix.c            # pthreads
│   │   └── thread_win32.c            # CreateThread + CRITICAL_SECTION
│   ├── server/
│   │   ├── start_stop.c              # Ciclo de vida del servidor
│   │   ├── connection.c              # Abstracción de lectura/escritura de socket
│   │   ├── connection_thread.c       # Manejador de conexiones por hilo
│   │   ├── http_request_parser.c
│   │   ├── request_handler.c         # Enrutamiento de peticiones
│   │   ├── static_handler.c          # Servido de archivos y listado de directorios
│   │   └── icons_handler.c           # Sirve los iconos embebidos en /_icons/
│   └── utils/
│       ├── config_loader.c
│       ├── server_utils.c            # Tipos MIME, URL encode/decode, motor de templates
│       └── log.h
├── html/                             # Templates HTML (embebidos en tiempo de compilación)
│   ├── dir_header.html
│   ├── dir_footer.html
│   └── error.html
├── icons/                            # Iconos GIF 16×16 (embebidos en tiempo de compilación)
├── configs/
│   └── config.txt
├── scripts/
│   ├── gen_icons_c.sh                # Embebe iconos en C (ejecutado por CMake)
│   ├── gen_html_c.sh                 # Embebe HTML en C (ejecutado por CMake)
│   └── ...
└── retroserver.sh                    # Punto de entrada principal (Linux/macOS)
```

---

## Autor

**Jonathan P. Toledo**  
[TheRetroCenter.com](https://www.theretrocenter.com)
