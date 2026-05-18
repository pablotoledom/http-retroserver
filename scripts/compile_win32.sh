#!/bin/bash
# Cross-compile for Windows 32-bit — compatible from Win95 to Win11.
# Requires: sudo apt install mingw-w64 python3

source ./scripts/show/welcome

user="$(whoami)"
echo "Hi ${user}!!"

source ./scripts/show/divbar
echo '1- Cleaning build directories...'
rm -rf ./build-win32 ./bin-win32

source ./scripts/show/divbar
echo '2- Compiling for Windows 32-bit (Win95 to Win11)...'

mkdir build-win32
cd build-win32
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-win95.cmake \
      -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..

mkdir -p bin-win32

source ./scripts/show/divbar
echo '3- Copying binary and configs...'

cp ./build-win32/retroserver.exe ./bin-win32/retroserver.exe
cp -r ./configs ./bin-win32/configs

source ./scripts/show/divbar
echo '4- Patching i686 instructions for i486/Win95 compatibility...'
gcc -O2 -o /tmp/patch_win32 ./scripts/patch_win32.c
/tmp/patch_win32 ./bin-win32/retroserver.exe
rm -f /tmp/patch_win32

source ./scripts/show/divbar
echo '5- Cleaning build directory...'
rm -rf ./build-win32

source ./scripts/show/divbar
echo "Done!! -> bin-win32/retroserver.exe"
