cmake_minimum_required(VERSION 3.10)
project(retroserver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# --- Embed icons at build time -------------------------------------------
file(GLOB ICON_GIF_FILES "${CMAKE_SOURCE_DIR}/icons/*.gif")

add_custom_command(
    OUTPUT  "${CMAKE_BINARY_DIR}/icons_data.c"
            "${CMAKE_BINARY_DIR}/icons_data.h"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/gen_icons_c.sh"
            "${CMAKE_SOURCE_DIR}/icons"
            "${CMAKE_BINARY_DIR}/icons_data.c"
            "${CMAKE_BINARY_DIR}/icons_data.h"
    DEPENDS ${ICON_GIF_FILES}
    COMMENT "Embedding icon GIFs into C source"
)

# --- Embed HTML templates at build time ----------------------------------
file(GLOB HTML_FILES "${CMAKE_SOURCE_DIR}/html/*.html")

add_custom_command(
    OUTPUT  "${CMAKE_BINARY_DIR}/html_data.c"
            "${CMAKE_BINARY_DIR}/html_data.h"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/gen_html_c.sh"
            "${CMAKE_SOURCE_DIR}/html"
            "${CMAKE_BINARY_DIR}/html_data.c"
            "${CMAKE_BINARY_DIR}/html_data.h"
    DEPENDS ${HTML_FILES}
    COMMENT "Embedding HTML templates into C source"
)
# -------------------------------------------------------------------------

set(SOURCES
    src/main.c
    src/server/start_stop.c
    src/server/connection.c
    src/server/connection_thread.c
    src/server/http_request_parser.c
    src/server/request_handler.c
    src/server/static_handler.c
    src/server/icons_handler.c
    src/utils/server_utils.c
    src/utils/config_loader.c
    "${CMAKE_BINARY_DIR}/icons_data.c"
    "${CMAKE_BINARY_DIR}/html_data.c"
)

add_executable(retroserver ${SOURCES})

target_include_directories(retroserver PRIVATE src "${CMAKE_BINARY_DIR}")

target_link_libraries(retroserver pthread)
