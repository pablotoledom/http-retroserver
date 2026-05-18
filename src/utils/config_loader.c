#define _XOPEN_SOURCE 700

#include "config_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int verbose_level = 3;
int http_port     = 8080;

int load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '#') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key   = line;
        char *value = eq + 1;
        value[strcspn(value, "\n\r")] = '\0';

        if      (strcmp(key, "verbose_level") == 0) verbose_level = atoi(value);
        else if (strcmp(key, "http_port")     == 0) http_port     = atoi(value);
    }

    fclose(file);
    return 0;
}
