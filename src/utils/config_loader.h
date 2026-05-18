#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

int load_config(const char *filename);

extern int verbose_level;
extern int http_port;

#endif // CONFIG_LOADER_H
