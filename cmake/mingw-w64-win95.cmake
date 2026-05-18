# Toolchain: cross-compile for Windows 95+ (32-bit, WinSock 1.1)
# Install: sudo apt install mingw-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_COMPILER   i686-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER  i686-w64-mingw32-windres)

# Enable Win95-compatible code paths (WinSock 1.1, FindFirstFile, etc.)
set(PLAT_WIN95 ON)
add_definitions(-DPLAT_WIN95)

# Target i486 CPU to avoid CMOVcc and other i686 instructions
# Disable MinGW stdio overrides so we use MSVCRT.DLL's printf (i486-safe)
# and avoid pulling in i686-compiled libmingwex.a
set(CMAKE_C_FLAGS   "-march=i486 -mtune=pentium -D__USE_MINGW_ANSI_STDIO=0")
# -static-libgcc: embed libgcc (avoids needing libgcc DLL)
# -Wl,--allow-multiple-definition: our win95_compat.c overrides take
# precedence over the i686-compiled versions in libmingw32/libgcc
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -Wl,--allow-multiple-definition")

set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
