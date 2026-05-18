#include "platform/platform.h"
#include "utils/config_loader.h"
#include "utils/log.h"
#include "server/start_stop.h"
#include "banner_data.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#ifndef PLAT_WINDOWS
#  include <errno.h>
#endif

int log_level = LOG_LEVEL_INFO;

static void handle_signal(int signum) {
    (void)signum;
    server_stop();
}

int main(int argc, char *argv[]) {
    if (plat_init() != 0) {
        fprintf(stderr, "Platform init failed\n");
        return EXIT_FAILURE;
    }

    printf("%s", BANNER);
    fflush(stdout);

    SIGPIPE_IGNORE();

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <root_directory>\n", argv[0]);
        fprintf(stderr, "  Serves files from <root_directory> over HTTP.\n");
        fprintf(stderr, "  Config: ./configs/config.txt (optional)\n");
        plat_cleanup();
        return EXIT_FAILURE;
    }

    char resolved[PATH_MAX];
    if (!plat_realpath(argv[1], resolved)) {
        fprintf(stderr, "Invalid root directory: %s\n", argv[1]);
        plat_cleanup();
        return EXIT_FAILURE;
    }

    if (load_config("./configs/config.txt") != 0)
        fprintf(stderr, "Note: config.txt not found, using defaults\n");

    log_level = verbose_level;

    printf("  Serving : %s\n", resolved);
    printf("  Port    : %d\n", http_port);
    printf("  Press Ctrl+C to stop.\n\n");
    fflush(stdout);

    LOG_INFO("HTTP RetroServer starting");
    LOG_INFO("  Root: %s", resolved);
    LOG_INFO("  HTTP port: %d", http_port);

#ifndef PLAT_WINDOWS
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);
#else
    /* Windows console Ctrl+C */
    signal(SIGINT, handle_signal);
#endif

    server_start(resolved, http_port);

    LOG_INFO("Server stopped.");
    plat_cleanup();
    return EXIT_SUCCESS;
}
