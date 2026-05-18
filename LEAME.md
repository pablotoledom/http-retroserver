# retroserver

> [English Version](README.md)

Servidor HTTP/HTTPS de archivos estáticos ligero, escrito en C. Diseñado para compatibilidad retro — el listado de directorios funciona en navegadores tan antiguos como Internet Explorer 3.0 y Lynx con una estética de terminal fósforo verde.

---

## Características

- Sirve archivos estáticos con un listado de directorios autogenerado
- HTTPS opcional mediante OpenSSL (TLS)
- Descarga forzada de cualquier archivo con el parámetro `?dl`
- Detección de tipo MIME para formatos de archivo comunes
- Manejo de conexiones multi-hilo
- Protección contra path traversal (bloquea `..` y escapes por symlink)
- Configurable mediante un archivo de texto plano
- Apagado graceful con `SIGINT` / `SIGTERM`
- Instalable como servicio systemd
- **Retro compatible:** listado de directorios probado hasta IE 3.0
- **UI fósforo verde:** fondo negro, estilo terminal verde
- **Binario autocontenido:** iconos y templates HTML se embeben en tiempo de compilación — no se necesitan archivos adicionales en producción

## Requisitos

- GCC o Clang
- CMake 3.10+
- OpenSSL (`libssl-dev`)

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake libssl-dev
```

## Compilación

```bash
# Build de producción
./retroserver.sh compile

# Build de debug (con símbolos de depuración)
./retroserver.sh compiledebug
```

El binario queda en `bin/retroserver` junto con `configs/` y `ssl/`. Los iconos y templates HTML se compilan directamente dentro del binario — no se necesitan carpetas adicionales en runtime.

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

Una vez iniciado, abrir el navegador en:

- HTTP:  `http://localhost:8080`
- HTTPS: `https://localhost:8443` (cuando SSL está habilitado)

## Configuración

Editar `configs/config.txt`:

```ini
# Nivel de log: 0=ninguno 1=error 2=warn 3=info 4=debug
verbose_level=3

# Puerto HTTP
http_port=8080

# HTTPS (opcional)
https_port=8443
ssl_enabled=0
ssl_cert=./ssl/cert.pem
ssl_key=./ssl/key.pem
```

Cambiar `ssl_enabled=1` para activar HTTPS.

## SSL / HTTPS

**Generar un certificado autofirmado:**

```bash
./retroserver.sh cert
```

Esto crea `ssl/cert.pem` y `ssl/key.pem`. Luego habilitar SSL en el config:

```ini
ssl_enabled=1
```

> **Nota:** Los certificados autofirmados mostrarán una advertencia de seguridad en el navegador. Se puede aceptar manualmente o agregar el certificado al almacén de confianza del sistema.

## Personalizar la UI

### Templates HTML

La estructura de las páginas vive en `html/`. Editar estos archivos y recompilar para aplicar cambios:

| Archivo | Descripción | Variables |
|---------|-------------|-----------|
| `html/dir_header.html` | Cabecera de página y apertura del listado | `{{PATH}}` |
| `html/dir_footer.html` | Pie de página | *(ninguna)* |
| `html/error.html` | Página de error | `{{CODE}}`, `{{STATUS}}` |

En tiempo de compilación, estos archivos se embeben automáticamente en el binario. No requieren deployment separado.

### Iconos

Los iconos son imágenes GIF de 16×16 en `icons/`, generados por `scripts/gen_icons.py`. Para personalizar:

1. Editar `scripts/gen_icons.py` (o reemplazar cualquier `.gif` en `icons/` por uno propio)
2. Recompilar — CMake detecta el cambio y re-embebe los iconos

| Icono | Tipos de archivo |
|-------|-----------------|
| Carpeta | Directorios |
| Sonido | mp3, ogg, wav, flac, aac, m4a, wma |
| Video | mp4, mkv, avi, mov, webm, flv, wmv |
| Imagen | jpg, jpeg, png, gif, svg, webp, ico, bmp |
| Archivo | zip, gz, tar, 7z, rar, bz2, xz |
| PDF | pdf |
| Disco | iso, cue |
| Ejecutable | exe, bin |
| Texto | txt, md, log, csv, rtf |
| Código fuente | c, h, cpp, py, sh, js, json, html, xml, go, rs, yaml |
| Genérico | Todo lo demás |

## Servicio Systemd

```bash
# Instalar como servicio
sudo ./retroserver.sh install

# Eliminar el servicio
sudo ./retroserver.sh uninstall
```

## Comandos Disponibles

| Comando        | Descripción                                     |
|----------------|-------------------------------------------------|
| `compile`      | Compilar para producción (Release)              |
| `compiledebug` | Compilar con símbolos de debug                  |
| `run [dir]`    | Iniciar el servidor (por defecto: `.`)          |
| `cert`         | Generar un certificado SSL autofirmado          |
| `install`      | Instalar como servicio systemd                  |
| `uninstall`    | Eliminar el servicio systemd                    |

## Estructura del Proyecto

```
retroserver/
├── src/
│   ├── main.c
│   ├── server/
│   │   ├── start_stop.c          # Ciclo de vida del servidor
│   │   ├── connection.c          # Abstracción de socket (plano + TLS)
│   │   ├── connection_thread.c   # Manejador de conexiones por hilo
│   │   ├── http_request_parser.c
│   │   ├── request_handler.c     # Enrutamiento de peticiones
│   │   ├── static_handler.c      # Servido de archivos y listado de directorios
│   │   ├── icons_handler.c       # Sirve los iconos embebidos en /_icons/
│   │   └── ssl_manager.c         # Configuración del contexto OpenSSL
│   └── utils/
│       ├── config_loader.c
│       ├── server_utils.c        # Tipos MIME, URL encode/decode, motor de templates
│       └── log.h
├── html/                         # Templates HTML (embebidos en tiempo de compilación)
│   ├── dir_header.html
│   ├── dir_footer.html
│   └── error.html
├── icons/                        # Iconos GIF (embebidos en tiempo de compilación)
├── configs/
│   └── config.txt
├── ssl/                          # Archivos de certificado (generados)
├── scripts/
│   ├── gen_icons.py              # Regenera los iconos (requiere Pillow)
│   ├── gen_icons_c.sh            # Embebe iconos en C (ejecutado por CMake)
│   ├── gen_html_c.sh             # Embebe HTML en C (ejecutado por CMake)
│   └── ...
└── retroserver.sh                # Punto de entrada principal
```

---

## Autor

**Jonathan P. Toledo**  
[TheRetroCenter.com](https://www.theretrocenter.com)
