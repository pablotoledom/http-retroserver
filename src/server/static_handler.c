// static_handler.c

#include "static_handler.h"
#include "connection.h"
#include "../utils/server_utils.h"
#include "../utils/log.h"
#include "../platform/platform.h"
#include "../platform/fs.h"
#include "html_data.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#ifndef PLAT_WINDOWS
#  include <fcntl.h>
#  include <unistd.h>
#  include <sys/stat.h>
#else
#  include <windows.h>
#  include <io.h>
#endif

#define CHUNK_SIZE      4096
#define MAX_DIR_ENTRIES 8192

typedef struct {
    char      name[512];
    int       is_dir;
    long long size;
    long      mtime;
} DirEntry;

typedef struct { const char *gif; const char *alt; } IconInfo;

static IconInfo get_icon(const char *name, int is_dir) {
    /* lbl uses &nbsp; padding so text browsers (Lynx) preserve column width */
    if (is_dir) return (IconInfo){"folder.gif", "[FOLDER]"};

    const char *dot = strrchr(name, '.');
    if (!dot) return (IconInfo){"file.gif", "[FILE]&nbsp;&nbsp;"};
    dot++;

    if (!strcasecmp(dot,"mp3") || !strcasecmp(dot,"ogg") || !strcasecmp(dot,"wav") ||
        !strcasecmp(dot,"flac")|| !strcasecmp(dot,"aac") || !strcasecmp(dot,"m4a") ||
        !strcasecmp(dot,"wma")) return (IconInfo){"sound.gif", "[MUSIC]&nbsp;"};

    if (!strcasecmp(dot,"mp4") || !strcasecmp(dot,"mkv") || !strcasecmp(dot,"avi") ||
        !strcasecmp(dot,"mov") || !strcasecmp(dot,"webm")|| !strcasecmp(dot,"flv") ||
        !strcasecmp(dot,"wmv")) return (IconInfo){"video.gif", "[VIDEO]&nbsp;"};

    if (!strcasecmp(dot,"jpg") || !strcasecmp(dot,"jpeg")|| !strcasecmp(dot,"png") ||
        !strcasecmp(dot,"gif") || !strcasecmp(dot,"svg") || !strcasecmp(dot,"webp")||
        !strcasecmp(dot,"ico") || !strcasecmp(dot,"bmp")) return (IconInfo){"image.gif", "[IMAGE]&nbsp;"};

    if (!strcasecmp(dot,"zip") || !strcasecmp(dot,"gz")  || !strcasecmp(dot,"tar") ||
        !strcasecmp(dot,"7z")  || !strcasecmp(dot,"rar") || !strcasecmp(dot,"bz2") ||
        !strcasecmp(dot,"xz")) return (IconInfo){"archive.gif", "[ZIPED]&nbsp;"};

    if (!strcasecmp(dot,"pdf")) return (IconInfo){"pdf.gif",  "[PDF]&nbsp;&nbsp;&nbsp;"};

    if (!strcasecmp(dot,"iso") || !strcasecmp(dot,"cue")) return (IconInfo){"iso.gif", "[DISC]&nbsp;&nbsp;"};

    if (!strcasecmp(dot,"exe") || !strcasecmp(dot,"bin")) return (IconInfo){"exe.gif", "[PROG]&nbsp;&nbsp;"};

    if (!strcasecmp(dot,"txt") || !strcasecmp(dot,"md")  || !strcasecmp(dot,"log") ||
        !strcasecmp(dot,"csv") || !strcasecmp(dot,"rtf")) return (IconInfo){"text.gif", "[TEXT]&nbsp;&nbsp;"};

    if (!strcasecmp(dot,"html")|| !strcasecmp(dot,"htm") || !strcasecmp(dot,"css") ||
        !strcasecmp(dot,"js")  || !strcasecmp(dot,"json")|| !strcasecmp(dot,"xml") ||
        !strcasecmp(dot,"c")   || !strcasecmp(dot,"h")   || !strcasecmp(dot,"cpp") ||
        !strcasecmp(dot,"py")  || !strcasecmp(dot,"sh")  || !strcasecmp(dot,"rs")  ||
        !strcasecmp(dot,"go")  || !strcasecmp(dot,"yaml")|| !strcasecmp(dot,"yml"))
        return (IconInfo){"src.gif", "[CODE]&nbsp;&nbsp;"};

    return (IconInfo){"file.gif", "[FILE]&nbsp;&nbsp;"};
}

static int cmp_entry(const void *a, const void *b) {
    const DirEntry *ea = (const DirEntry *)a;
    const DirEntry *eb = (const DirEntry *)b;
    if (ea->is_dir != eb->is_dir) return eb->is_dir - ea->is_dir; // dirs first
    return strcmp(ea->name, eb->name);
}

static void fmt_size(char *buf, size_t buf_size, long long sz) {
#define FMT_FRAC(val, unit) \
    (long)((val) / (unit)), (long)(((val) % (unit)) * 10 / (unit))
    if (sz < 1024)
        snprintf(buf, buf_size, "%ld", (long)sz);
    else if (sz < 1024 * 1024)
        snprintf(buf, buf_size, "%ld.%ldK", FMT_FRAC(sz, 1024));
    else if (sz < (long long)1024 * 1024 * 1024)
        snprintf(buf, buf_size, "%ld.%ldM", FMT_FRAC(sz, 1024*1024));
    else
        snprintf(buf, buf_size, "%ld.%ldG", FMT_FRAC(sz, (long long)1024*1024*1024));
#undef FMT_FRAC
}

static void send_status(void *ctx, int code, const char *status) {
    /* HTTP header — no Content-Length, browser reads until close */
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n\r\n",
        code, status);
    connection_write(ctx, hdr, strlen(hdr));

    /* Body from error.html template */
    char code_str[8];
    snprintf(code_str, sizeof(code_str), "%d", code);
    const char *keys[] = { "CODE", "STATUS" };
    const char *vals[] = { code_str, status };
    int tlen = 0;
    const char *tmpl = html_get("error.html", &tlen);
    char body[1024];
    int blen = tmpl_append(body, (int)sizeof(body), 0, tmpl, tlen, keys, vals, 2);
    if (blen > 0) connection_write(ctx, body, (size_t)blen);
}

// -----------------------------------------------------------------------
// Directory listing — streamed directly, no Content-Length
// -----------------------------------------------------------------------
static void send_directory_listing(void *ctx, const char *safe_path,
                                   const char *url_path) {
    fs_dir_t *dir = fs_opendir(safe_path);
    if (!dir) { send_status(ctx, 403, "Forbidden"); return; }

    DirEntry *entries = (DirEntry *)malloc(MAX_DIR_ENTRIES * sizeof(DirEntry));
    if (!entries) { fs_closedir(dir); send_status(ctx, 500, "Internal Server Error"); return; }

    int count = 0;
    fs_entry_t fent;
    while (fs_readdir(dir, &fent) == 1 && count < MAX_DIR_ENTRIES) {
        snprintf(entries[count].name, sizeof(entries[count].name), "%s", fent.name);
        entries[count].is_dir = fent.is_dir;
        entries[count].size   = fent.size;
        entries[count].mtime  = fent.mtime;
        count++;
    }
    fs_closedir(dir);
    qsort(entries, count, sizeof(DirEntry), cmp_entry);

    /* HTTP header — no Content-Length; browser reads until connection close */
    {
        const char *hdr =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Connection: close\r\n\r\n";
        connection_write(ctx, hdr, strlen(hdr));
    }

    /* HTML header template */
    {
        char buf[4096];
        char esc_url[512];
        html_encode(esc_url, url_path, sizeof(esc_url));
        const char *keys[] = { "PATH" };
        const char *vals[] = { esc_url };
        int tlen = 0;
        const char *tmpl = html_get("dir_header.html", &tlen);
        int n = tmpl_append(buf, (int)sizeof(buf), 0, tmpl, tlen, keys, vals, 1);
        if (n > 0) connection_write(ctx, buf, (size_t)n);
    }

    /* Parent directory link */
    if (strcmp(url_path, "/") != 0) {
        const char *row =
            "<tr>"
            "<td><img src=\"/_icons/back.gif\" width=16 height=16 border=0 alt=\"[PARENT]\"></td>"
            "<td><a href=\"../\">../</a></td>"
            "<td>-</td><td align=\"right\">-</td><td></td>"
            "</tr>\n";
        connection_write(ctx, row, strlen(row));
    }

    /* One row per entry — small stack buffer, sent immediately */
    for (int i = 0; i < count; i++) {
        char row[2048];
        char esc_name[512];
        char url_name[512];
        char date_str[24];
        char size_str[24];
        int  n;

        html_encode(esc_name, entries[i].name, sizeof(esc_name));
        url_encode(url_name,  entries[i].name, sizeof(url_name));

        struct tm tm_info;
        time_t t = (time_t)entries[i].mtime;
        plat_localtime(&t, &tm_info);
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", &tm_info);

        IconInfo icon = get_icon(entries[i].name, entries[i].is_dir);

        if (entries[i].is_dir) {
            char display[513];
            char href[768];
            snprintf(display, sizeof(display), "%s/", esc_name);
            snprintf(href,    sizeof(href),    "%s/", url_name);
            n = snprintf(row, sizeof(row),
                "<tr>"
                "<td><img src=\"/_icons/%s\" width=16 height=16 border=0 alt=\"%s\"></td>"
                "<td><a href=\"%s\">%s</a></td>"
                "<td>%s</td><td align=\"right\">-</td><td></td>"
                "</tr>\n",
                icon.gif, icon.alt, href, display, date_str);
        } else {
            fmt_size(size_str, sizeof(size_str), entries[i].size);
            n = snprintf(row, sizeof(row),
                "<tr>"
                "<td><img src=\"/_icons/%s\" width=16 height=16 border=0 alt=\"%s\"></td>"
                "<td><a href=\"%s\">%s</a></td>"
                "<td>%s</td>"
                "<td align=\"right\" class=\"sz\">%s</td>"
                "<td><a href=\"%s?dl\">[dl]</a></td>"
                "</tr>\n",
                icon.gif, icon.alt, url_name, esc_name, date_str, size_str, url_name);
        }
        if (n > 0) connection_write(ctx, row, (size_t)n);
    }

    /* HTML footer template */
    {
        char buf[512];
        int tlen = 0;
        const char *tmpl = html_get("dir_footer.html", &tlen);
        int n = tmpl_append(buf, (int)sizeof(buf), 0, tmpl, tlen, NULL, NULL, 0);
        if (n > 0) connection_write(ctx, buf, (size_t)n);
    }

    free(entries);
}

// -----------------------------------------------------------------------
// File serving
// -----------------------------------------------------------------------
typedef struct { long long size; int is_dir; } file_info_t;

static int get_file_info(const char *path, file_info_t *info) {
#ifdef PLAT_WINDOWS
    /* FindFirstFile works on Win95+, unlike GetFileAttributesEx (Win98+/NT4) */
    WIN32_FIND_DATA d;
    HANDLE h = FindFirstFile(path, &d);
    if (h == INVALID_HANDLE_VALUE) return -1;
    FindClose(h);
    info->is_dir = (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info->size   = ((long long)d.nFileSizeHigh << 32) | d.nFileSizeLow;
#else
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    info->is_dir = S_ISDIR(st.st_mode);
    info->size   = (long long)st.st_size;
#endif
    return 0;
}

static void serve_file(void *ctx, const char *safe_path,
                       long long file_size, int force_download) {
#ifdef PLAT_WINDOWS
    HANDLE fh = CreateFile(safe_path, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) { send_status(ctx, 500, "Internal Server Error"); return; }
#else
    int fd = open(safe_path, O_RDONLY);
    if (fd < 0) { send_status(ctx, 500, "Internal Server Error"); return; }
#endif

    const char *mime  = get_mime_type(safe_path);
    const char *slash = strrchr(safe_path, '/');
    if (!slash) slash = strrchr(safe_path, '\\');
    const char *fname = slash ? slash + 1 : safe_path;

    char hdr[1024];
    if (force_download) {
        char url_fname[512];
        url_encode(url_fname, fname, sizeof(url_fname));
        snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Length: %ld\r\n"
            "Content-Disposition: attachment; filename=\"%s\"\r\n"
            "Connection: close\r\n\r\n",
            (long)file_size, url_fname);
    } else {
        snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            mime, (long)file_size);
    }
    connection_write(ctx, hdr, strlen(hdr));

    char buf[CHUNK_SIZE];
#ifdef PLAT_WINDOWS
    DWORD n;
    while (ReadFile(fh, buf, sizeof(buf), &n, NULL) && n > 0)
        connection_write(ctx, buf, (size_t)n);
    CloseHandle(fh);
#else
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        connection_write(ctx, buf, (size_t)n);
    close(fd);
#endif
}

// -----------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------
void handle_static_file_or_directory(void *ctx, const char *root_directory,
                                      const char *decoded_url, int force_download) {
    if (strstr(decoded_url, "..")) {
        LOG_WARN("Rejecting path with '..': %s", decoded_url);
        send_status(ctx, 403, "Forbidden");
        return;
    }

    char candidate[PATH_MAX];
    if (*decoded_url == '\0' || strcmp(decoded_url, "/") == 0) {
        send_directory_listing(ctx, root_directory, "/");
        return;
    }

    /* Build candidate path:
     * 1. Strip leading '/' from url if root already ends with separator
     *    (avoids "D:\\/subdir" on Win95)
     * 2. Strip trailing '/' from url
     *    (avoids "D:\DRIVERS/" — Win95 APIs don't handle trailing slash well) */
    {
        size_t rlen = strlen(root_directory);
        const char *url = decoded_url;
        if (rlen > 0 && is_path_sep(root_directory[rlen - 1]) && is_path_sep(url[0]))
            url++;

        /* Copy and strip any trailing path separator */
        char url_clean[PATH_MAX];
        size_t ulen = strlen(url);
        if (ulen >= PATH_MAX) ulen = PATH_MAX - 1;
        memcpy(url_clean, url, ulen);
        url_clean[ulen] = '\0';
        while (ulen > 0 && is_path_sep(url_clean[ulen - 1]))
            url_clean[--ulen] = '\0';

        snprintf(candidate, sizeof(candidate), "%s%s", root_directory, url_clean);
    }

    file_info_t info;
    if (get_file_info(candidate, &info) != 0) {
        send_status(ctx, 404, "Not Found");
        return;
    }

    char safe_path[PATH_MAX];
    if (!plat_realpath(candidate, safe_path)) {
        send_status(ctx, 403, "Forbidden");
        return;
    }
    size_t root_len = strlen(root_directory);
    /* If root_directory ends with a separator (e.g. "C:\"), the next char
     * in safe_path is the first char of the entry name — not a separator.
     * We must skip the separator check in that case. */
    int root_ends_sep = (root_len > 0 && is_path_sep(root_directory[root_len - 1]));
    char sep = safe_path[root_len];
    if (strncmp(safe_path, root_directory, root_len) != 0 ||
        (!root_ends_sep && !is_path_sep(sep) && sep != '\0')) {
        LOG_WARN("Path traversal blocked: %s", decoded_url);
        send_status(ctx, 403, "Forbidden");
        return;
    }

    if (info.is_dir) {
        size_t url_len = strlen(decoded_url);
        if (url_len > 0 && decoded_url[url_len - 1] != '/') {
            char redirect[512];
            snprintf(redirect, sizeof(redirect),
                "HTTP/1.1 301 Moved Permanently\r\n"
                "Location: %s/\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n",
                decoded_url);
            connection_write(ctx, redirect, strlen(redirect));
            return;
        }
        send_directory_listing(ctx, safe_path, decoded_url);
    } else {
        serve_file(ctx, safe_path, info.size, force_download);
    }
}
