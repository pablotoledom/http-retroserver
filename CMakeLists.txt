cmake_minimum_required(VERSION 3.10)
project(retroserver C)

set(CMAKE_C_STANDARD 99)
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

# --- Embed startup banner at build time ----------------------------------
add_custom_command(
    OUTPUT  "${CMAKE_BINARY_DIR}/banner_data.c"
            "${CMAKE_BINARY_DIR}/banner_data.h"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/gen_banner_c.sh"
            "${CMAKE_SOURCE_DIR}/scripts/show/welcome"
            "${CMAKE_BINARY_DIR}/banner_data.c"
            "${CMAKE_BINARY_DIR}/banner_data.h"
    DEPENDS "${CMAKE_SOURCE_DIR}/scripts/show/welcome"
    COMMENT "Embedding startup banner"
)
# -------------------------------------------------------------------------

# --- Platform-specific sources -------------------------------------------
if(WIN32)
    if(PLAT_WIN95)
        set(PLATFORM_SOURCES
            src/platform/fs_win32.c
            src/platform/thread_win32.c
            src/platform/win95_compat.c
        )
    else()
        set(PLATFORM_SOURCES
            src/platform/fs_win32.c
            src/platform/thread_win32.c
        )
    endif()
else()
    set(PLATFORM_SOURCES
        src/platform/fs_posix.c
        src/platform/thread_posix.c
    )
endif()
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
    ${PLATFORM_SOURCES}
    "${CMAKE_BINARY_DIR}/icons_data.c"
    "${CMAKE_BINARY_DIR}/html_data.c"
    "${CMAKE_BINARY_DIR}/banner_data.c"
)

add_executable(retroserver ${SOURCES})

target_include_directories(retroserver PRIVATE src "${CMAKE_BINARY_DIR}")

# --- Platform-specific linking -------------------------------------------
if(WIN32)
    # Static link so the .exe runs without extra DLLs on the target machine
    if(PLAT_WIN95)
        # Win95: use system DLLs (msvcrt.dll, wsock32.dll, kernel32.dll)
        # These are always present on Win95 and are compiled for i486
        target_link_libraries(retroserver wsock32)
    else()
        # Win98+: static link so the .exe needs no extra DLLs
        target_link_libraries(retroserver ws2_32 -static)
    endif()
else()
    target_link_libraries(retroserver pthread)
endif()
