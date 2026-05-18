// ssl_manager.h

#ifndef SSL_MANAGER_H
#define SSL_MANAGER_H

#include <openssl/ssl.h>

SSL_CTX *ssl_create_context(const char *cert_path, const char *key_path);
void ssl_free_context(SSL_CTX *ctx);

#endif // SSL_MANAGER_H
