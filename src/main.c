#define _XOPEN_SOURCE 700

#include "utils/config_loader.h"
#include "utils/log.h"
#include "server/start_stop.h"
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int log_level = LOG_LEVEL_INFO;

static void handle_signal(int signum) {
    (void)signum;
    server_stop();
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <root_directory>\n", argv[0]);
        fprintf(stderr, "  Serves files from <root_directory> over HTTP.\n");
        fprintf(stderr, "  Config: ./configs/config.txt (optional)\n");
        return EXIT_FAILURE;
    }

    char resolved[PATH_MAX];
    if (!realpath(argv[1], resolved)) {
        fprintf(stderr, "Invalid root directory '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    // Load config (non-fatal if missing)
    if (load_config("./configs/config.txt") != 0)
        fprintf(stderr, "Note: ./configs/config.txt not found, using defaults\n");

    log_level = verbose_level;

    LOG_INFO("HTTP RetroServer starting");
    LOG_INFO("  Root: %s", resolved);
    LOG_INFO("  HTTP port: %d", http_port);
    if (ssl_enabled)
        LOG_INFO("  HTTPS port: %d", https_port);

    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    server_start(resolved, ssl_enabled, ssl_cert, ssl_key, http_port, https_port);

    LOG_INFO("Server stopped.");
    return EXIT_SUCCESS;
}
