// ssl_manager.c

#include "ssl_manager.h"
#include "../utils/log.h"
#include <openssl/err.h>

SSL_CTX *ssl_create_context(const char *cert_path, const char *key_path) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        LOG_ERROR("Failed to create SSL context");
        return NULL;
    }
    if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
        LOG_ERROR("Failed to load SSL certificate: %s", cert_path);
        SSL_CTX_free(ctx);
        return NULL;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
        LOG_ERROR("Failed to load SSL private key: %s", key_path);
        SSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

void ssl_free_context(SSL_CTX *ctx) {
    if (ctx) SSL_CTX_free(ctx);
}
