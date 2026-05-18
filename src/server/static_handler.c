// static_handler.c

#define _XOPEN_SOURCE 700

#include "static_handler.h"
#include "connection.h"
#include "../utils/server_utils.h"
#include "../utils/log.h"
#include "html_data.h"
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define CHUNK_SIZE      4096
#define MAX_DIR_ENTRIES 8192
#define DIR_BUF_SIZE    (512 * 1024)

typedef struct {
    char   name[256];
    int    is_dir;
    off_t  size;
    time_t mtime;
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

static void fmt_size(char *buf, size_t buf_size, off_t sz) {
    if (sz < 1024)
        snprintf(buf, buf_size, "%ld", (long)sz);
    else if (sz < 1024 * 1024)
        snprintf(buf, buf_size, "%.1fK", (double)sz / 1024.0);
    else if (sz < (off_t)1024 * 1024 * 1024)
        snprintf(buf, buf_size, "%.1fM", (double)sz / (1024.0 * 1024.0));
    else
        snprintf(buf, buf_size, "%.1fG", (double)sz / (1024.0 * 1024.0 * 1024.0));
}

static void send_status(void *ctx, int code, const char *status) {
    char code_str[8];
    snprintf(code_str, sizeof(code_str), "%d", code);
    const char *keys[] = { "CODE", "STATUS" };
    const char *vals[] = { code_str, status };
    char body[256];
    int blen = tmpl_append(body, sizeof(body), 0,
                           html_get("error.html"), keys, vals, 2);
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        code, status, blen);
    connection_write(ctx, hdr, strlen(hdr));
    connection_write(ctx, body, blen);
}

// -----------------------------------------------------------------------
// Directory listing
// -----------------------------------------------------------------------
static void send_directory_listing(void *ctx, const char *safe_path,
                                   const char *url_path) {
    DIR *dir = opendir(safe_path);
    if (!dir) { send_status(ctx, 403, "Forbidden"); return; }

    DirEntry *entries = malloc(MAX_DIR_ENTRIES * sizeof(DirEntry));
    if (!entries) { closedir(dir); send_status(ctx, 500, "Internal Server Error"); return; }

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && count < MAX_DIR_ENTRIES) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", safe_path, ent->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        strncpy(entries[count].name, ent->d_name, 255);
        entries[count].name[255] = '\0';
        entries[count].is_dir  = S_ISDIR(st.st_mode);
        entries[count].size    = st.st_size;
        entries[count].mtime   = st.st_mtime;
        count++;
    }
    closedir(dir);

    qsort(entries, count, sizeof(DirEntry), cmp_entry);

    char *body = malloc(DIR_BUF_SIZE);
    if (!body) { free(entries); send_status(ctx, 500, "Internal Server Error"); return; }

    char esc_url[512];
    html_encode(esc_url, url_path, sizeof(esc_url));

    int len = 0;

    {
        const char *keys[] = { "PATH" };
        const char *vals[] = { esc_url };
        len = tmpl_append(body, DIR_BUF_SIZE, len,
                          html_get("dir_header.html"), keys, vals, 1);
    }

    // Parent directory link (unless at root)
    if (strcmp(url_path, "/") != 0) {
        len += snprintf(body + len, DIR_BUF_SIZE - len,
            "<tr>"
            "<td><img src=\"/_icons/back.gif\" width=16 height=16 border=0 alt=\"[PARENT]\"></td>"
            "<td><a href=\"../\">../</a></td>"
            "<td>-</td>"
            "<td align=\"right\">-</td>"
            "<td></td>"
            "</tr>\n");
    }

    for (int i = 0; i < count && len < DIR_BUF_SIZE - 512; i++) {
        char esc_name[512];
        char url_name[512];
        char date_str[24];
        char size_str[24];

        html_encode(esc_name, entries[i].name, sizeof(esc_name));
        url_encode(url_name, entries[i].name, sizeof(url_name));

        struct tm tm_info;
        localtime_r(&entries[i].mtime, &tm_info);
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", &tm_info);

        IconInfo icon = get_icon(entries[i].name, entries[i].is_dir);

        if (entries[i].is_dir) {
            snprintf(size_str, sizeof(size_str), "-");
            char display[513];
            snprintf(display, sizeof(display), "%s/", esc_name);
            char href[768];
            snprintf(href, sizeof(href), "%s/", url_name);
            len += snprintf(body + len, DIR_BUF_SIZE - len,
                "<tr>"
                "<td><img src=\"/_icons/%s\" width=16 height=16 border=0 alt=\"%s\"></td>"
                "<td><a href=\"%s\">%s</a></td>"
                "<td>%s</td>"
                "<td align=\"right\">%s</td>"
                "<td></td>"
                "</tr>\n",
                icon.gif, icon.alt, href, display, date_str, size_str);
        } else {
            fmt_size(size_str, sizeof(size_str), entries[i].size);
            len += snprintf(body + len, DIR_BUF_SIZE - len,
                "<tr>"
                "<td><img src=\"/_icons/%s\" width=16 height=16 border=0 alt=\"%s\"></td>"
                "<td><a href=\"%s\">%s</a></td>"
                "<td>%s</td>"
                "<td align=\"right\" class=\"sz\">%s</td>"
                "<td><a href=\"%s?dl\">[dl]</a></td>"
                "</tr>\n",
                icon.gif, icon.alt, url_name, esc_name, date_str, size_str, url_name);
        }
    }

    len = tmpl_append(body, DIR_BUF_SIZE, len,
                      html_get("dir_footer.html"), NULL, NULL, 0);

    char hdr[256];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        len);

    connection_write(ctx, hdr, strlen(hdr));
    connection_write(ctx, body, len);

    free(body);
    free(entries);
}

// -----------------------------------------------------------------------
// File serving
// -----------------------------------------------------------------------
static void serve_file(void *ctx, const char *safe_path,
                       const struct stat *st, int force_download) {
    int fd = open(safe_path, O_RDONLY);
    if (fd < 0) { send_status(ctx, 500, "Internal Server Error"); return; }

    const char *mime = get_mime_type(safe_path);
    const char *slash = strrchr(safe_path, '/');
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
            (long)st->st_size, url_fname);
    } else {
        snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            mime, (long)st->st_size);
    }

    connection_write(ctx, hdr, strlen(hdr));

    char buf[CHUNK_SIZE];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        connection_write(ctx, buf, (size_t)n);

    close(fd);
}

// -----------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------
void handle_static_file_or_directory(void *ctx, const char *root_directory,
                                      const char *decoded_url, int force_download) {
    // Reject obvious path traversal
    if (strstr(decoded_url, "..")) {
        LOG_WARN("Rejecting path with '..': %s", decoded_url);
        send_status(ctx, 403, "Forbidden");
        return;
    }

    // Build candidate path
    char candidate[PATH_MAX];
    if (*decoded_url == '\0' || strcmp(decoded_url, "/") == 0) {
        // Root: serve the root directory directly
        send_directory_listing(ctx, root_directory, "/");
        return;
    }
    snprintf(candidate, sizeof(candidate), "%s%s", root_directory, decoded_url);

    struct stat st;
    if (stat(candidate, &st) != 0) {
        send_status(ctx, 404, "Not Found");
        return;
    }

    // Verify the resolved path stays inside root (prevents symlink escapes)
    char safe_path[PATH_MAX];
    if (!realpath(candidate, safe_path)) {
        send_status(ctx, 403, "Forbidden");
        return;
    }
    size_t root_len = strlen(root_directory);
    if (strncmp(safe_path, root_directory, root_len) != 0 ||
        (safe_path[root_len] != '/' && safe_path[root_len] != '\0')) {
        LOG_WARN("Path traversal blocked: %s", decoded_url);
        send_status(ctx, 403, "Forbidden");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        // Redirect /dir → /dir/ so relative links work correctly
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
        serve_file(ctx, safe_path, &st, force_download);
    }
}
