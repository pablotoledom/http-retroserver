// static_handler.h

#ifndef STATIC_HANDLER_H
#define STATIC_HANDLER_H

// Sirve un archivo o lista un directorio.
// force_download=1 → Content-Disposition: attachment (fuerza descarga en el browser)
void handle_static_file_or_directory(void *ctx, const char *root_directory,
                                      const char *decoded_url, int force_download);

#endif // STATIC_HANDLER_H
