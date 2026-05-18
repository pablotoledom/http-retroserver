// server_utils.c

#define _XOPEN_SOURCE 700

#include "server_utils.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

const char *get_mime_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) return "text/html; charset=UTF-8";
    if (strcmp(dot, ".css")  == 0) return "text/css";
    if (strcmp(dot, ".js")   == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".txt")  == 0) return "text/plain; charset=UTF-8";
    if (strcmp(dot, ".xml")  == 0) return "text/xml";
    if (strcmp(dot, ".jpg")  == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".png")  == 0) return "image/png";
    if (strcmp(dot, ".gif")  == 0) return "image/gif";
    if (strcmp(dot, ".ico")  == 0) return "image/x-icon";
    if (strcmp(dot, ".svg")  == 0) return "image/svg+xml";
    if (strcmp(dot, ".webp") == 0) return "image/webp";
    if (strcmp(dot, ".mp4")  == 0) return "video/mp4";
    if (strcmp(dot, ".mp3")  == 0) return "audio/mpeg";
    if (strcmp(dot, ".ogg")  == 0) return "audio/ogg";
    if (strcmp(dot, ".pdf")  == 0) return "application/pdf";
    if (strcmp(dot, ".zip")  == 0) return "application/zip";
    if (strcmp(dot, ".gz")   == 0) return "application/gzip";
    if (strcmp(dot, ".woff") == 0) return "font/woff";
    if (strcmp(dot, ".woff2")== 0) return "font/woff2";
    return "application/octet-stream";
}

void url_encode(char *dst, const char *src, size_t dst_size) {
    char *dst_end = dst + dst_size - 1;
    while (*src && dst < dst_end) {
        if (isalnum((unsigned char)*src) || strchr("-_.~", *src)) {
            *dst++ = *src;
        } else {
            if (dst + 3 >= dst_end) break;
            snprintf(dst, dst_end - dst, "%%%02X", (unsigned char)*src);
            dst += 3;
        }
        src++;
    }
    *dst = '\0';
}

void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            a = (char)tolower(a);
            b = (char)tolower(b);
            a = (a >= 'a') ? a - 'a' + 10 : a - '0';
            b = (b >= 'a') ? b - 'a' + 10 : b - '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void html_encode(char *dst, const char *src, size_t dst_size) {
    char *dst_end = dst + dst_size - 1;
    while (*src && dst < dst_end) {
        if (*src == '&') {
            if (dst + 5 >= dst_end) break;
            strncpy(dst, "&amp;", dst_end - dst);
            dst += 5;
        } else if (*src == '<') {
            if (dst + 4 >= dst_end) break;
            strncpy(dst, "&lt;", dst_end - dst);
            dst += 4;
        } else if (*src == '>') {
            if (dst + 4 >= dst_end) break;
            strncpy(dst, "&gt;", dst_end - dst);
            dst += 4;
        } else if (*src == '"') {
            if (dst + 6 >= dst_end) break;
            strncpy(dst, "&quot;", dst_end - dst);
            dst += 6;
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

int sanitize_path(const char *url_path, char *safe_path, size_t size, const char *root_directory) {
    char path[PATH_MAX];
    if (snprintf(path, sizeof(path), "%s%s", root_directory, url_path) >= (int)sizeof(path))
        return 0;

    char resolved[PATH_MAX];
    if (realpath(path, resolved) == NULL)
        return 0;

    size_t root_len = strlen(root_directory);
    if (strncmp(resolved, root_directory, root_len) != 0)
        return 0;
    if (strlen(resolved) >= size)
        return 0;

    strcpy(safe_path, resolved);
    return 1;
}

int tmpl_append(char *buf, int buf_size, int off,
                const char *tmpl,
                const char * const *keys,
                const char * const *vals,
                int nkeys) {
    const char *p = tmpl;
    while (*p && off < buf_size - 1) {
        if (p[0] == '{' && p[1] == '{') {
            const char *end = strstr(p + 2, "}}");
            if (!end) { buf[off++] = *p++; continue; }
            size_t klen = (size_t)(end - (p + 2));
            int replaced = 0;
            for (int i = 0; i < nkeys; i++) {
                if (strlen(keys[i]) == klen && strncmp(keys[i], p + 2, klen) == 0) {
                    const char *v = vals[i];
                    while (*v && off < buf_size - 1)
                        buf[off++] = *v++;
                    replaced = 1;
                    break;
                }
            }
            if (!replaced) {
                const char *q = p;
                while (q < end + 2 && off < buf_size - 1)
                    buf[off++] = *q++;
            }
            p = end + 2;
        } else {
            buf[off++] = *p++;
        }
    }
    buf[off] = '\0';
    return off;
}
