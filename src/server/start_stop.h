// start_stop.h

#ifndef START_STOP_H
#define START_STOP_H

#include <openssl/ssl.h>

int  server_start(const char *root_dir, int ssl_enabled,
                  const char *ssl_cert, const char *ssl_key,
                  int http_port, int https_port);
void server_stop(void);

#endif // START_STOP_H
