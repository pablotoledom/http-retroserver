// server_utils.c

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stddef.h>

const char *get_mime_type(const char *path);
void url_encode(char *dst, const char *src, size_t dst_size);
void url_decode(char *dst, const char *src);
void html_encode(char *dst, const char *src, size_t dst_size);
int sanitize_path(const char *url_path, char *safe_path, size_t size, const char *root_directory);

/*
 * Append template tmpl to buf[off..], replacing {{KEY}} tokens with vals[i].
 * Returns the new offset. Safe: never writes past buf_size.
 */
int tmpl_append(char *buf, int buf_size, int off,
                const char *tmpl,
                const char * const *keys,
                const char * const *vals,
                int nkeys);

#endif // SERVER_UTILS_H
