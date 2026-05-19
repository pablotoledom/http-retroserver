#!/bin/bash
# Run this script ON a Mac (not cross-compiled from Linux).
# Requirements: cmake (brew install cmake)

source ./scripts/show/welcome

user="$(whoami)"
echo "Hi ${user}!!"

source ./scripts/show/divbar
echo '1- Cleaning build directories...'
rm -rf ./build ./bin-macos

source ./scripts/show/divbar
echo '2- Compiling for macOS (Release)...'

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..

mkdir -p bin-macos

source ./scripts/show/divbar
echo '3- Copying binary and configs...'

cp ./build/retroserver ./bin-macos/retroserver
cp -r ./configs ./bin-macos/configs

source ./scripts/show/divbar
echo '4- Cleaning build directory...'
rm -rf ./build

source ./scripts/show/divbar
echo "Done!! -> bin-macos/retroserver"
