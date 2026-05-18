// request_handler.c
#define _XOPEN_SOURCE 700

#include "request_handler.h"
#include "connection.h"
#include "http_request_parser.h"
#include "static_handler.h"
#include "icons_handler.h"
#include "../utils/server_utils.h"
#include "../utils/log.h"
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#define RAW_REQUEST_SIZE (16 * 1024)

static const char *get_header_val(HttpRequest *req, const char *key) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0)
            return req->headers[i].value;
    }
    return NULL;
}

void request_handler(read_func_t read_func, void *ctx, const char *root_directory) {
    char *raw = malloc(RAW_REQUEST_SIZE);
    if (!raw) { connection_close(ctx); return; }

    HttpRequest req;
    memset(&req, 0, sizeof(req));

    // Read until end of headers
    int total = 0;
    while (total < RAW_REQUEST_SIZE - 1) {
        int n = read_func(ctx, raw + total, RAW_REQUEST_SIZE - 1 - total);
        if (n <= 0) goto cleanup;
        total += n;
        raw[total] = '\0';
        if (strstr(raw, "\r\n\r\n")) break;
    }

    if (total == 0) goto cleanup;

    if (parse_http_request(raw, total, &req) != 0) {
        const char *resp =
            "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        connection_write(ctx, resp, strlen(resp));
        goto cleanup;
    }

    (void)get_header_val; // suppress unused warning if not used elsewhere

    LOG_INFO("%s %s", req.method, req.url);

    if (strcmp(req.method, "GET") != 0 && strcmp(req.method, "HEAD") != 0) {
        const char *resp =
            "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET, HEAD\r\n"
            "Content-Length: 0\r\nConnection: close\r\n\r\n";
        connection_write(ctx, resp, strlen(resp));
        goto cleanup;
    }

    // Split path from query string
    char path[256];
    strncpy(path, req.url, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    // Check for ?dl query param (force download)
    int force_download = 0;
    char *qs = strchr(path, '?');
    if (qs) {
        if (strstr(qs + 1, "dl") != NULL) force_download = 1;
        *qs = '\0';
    }

    // URL-decode the path
    char decoded[256];
    url_decode(decoded, path);

    // Built-in icon route
    if (strncmp(decoded, "/_icons/", 8) == 0) {
        serve_icon(ctx, decoded + 8);
        goto cleanup;
    }

    handle_static_file_or_directory(ctx, root_directory, decoded, force_download);

cleanup:
    free(raw);
    if (req.body) free(req.body);
    connection_close(ctx);
}
