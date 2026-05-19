#!/bin/bash
source ./scripts/show/welcome

user="$(whoami)"
echo "Hi ${user}!!"

source ./scripts/show/divbar
echo '1- Cleaning build directories...'
rm -rf ./build ./bin-linux

source ./scripts/show/divbar
echo '2- Compiling for Linux (Release)...'

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ..

mkdir -p bin-linux

source ./scripts/show/divbar
echo '3- Copying binary and configs...'

cp ./build/retroserver ./bin-linux/retroserver
cp -r ./configs ./bin-linux/configs

source ./scripts/show/divbar
echo '4- Cleaning build directory...'
rm -rf ./build

source ./scripts/show/divbar
echo "Done!! -> bin-linux/retroserver"
