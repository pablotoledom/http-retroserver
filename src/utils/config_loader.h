// config_loader.h

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <stdbool.h>

int load_config(const char *filename);

extern int verbose_level;
extern int http_port;
extern int https_port;
extern bool ssl_enabled;
extern char ssl_cert[256];
extern char ssl_key[256];

#endif // CONFIG_LOADER_H
