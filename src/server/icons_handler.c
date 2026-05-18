#include "icons_handler.h"
#include "icons_data.h"
#include "connection.h"
#include <string.h>
#include <stdio.h>

void serve_icon(void *ctx, const char *filename) {
    if (strchr(filename, '/') || strchr(filename, '\\') || strstr(filename, "..")) {
        const char *r = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        connection_write(ctx, r, strlen(r));
        return;
    }

    for (int i = 0; icon_table[i].name != NULL; i++) {
        if (strcmp(icon_table[i].name, filename) == 0) {
            char hdr[256];
            snprintf(hdr, sizeof(hdr),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: image/gif\r\n"
                "Content-Length: %u\r\n"
                "Cache-Control: max-age=86400\r\n"
                "Connection: close\r\n\r\n",
                icon_table[i].len);
            connection_write(ctx, hdr, strlen(hdr));
            connection_write(ctx, (const char *)icon_table[i].data, icon_table[i].len);
            return;
        }
    }

    const char *r = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    connection_write(ctx, r, strlen(r));
}
