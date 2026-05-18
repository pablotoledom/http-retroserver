#!/bin/bash
source ./scripts/show/welcome

user="$(whoami)"
echo "Hi ${user}!!"

source ./scripts/show/divbar
echo '1- Cleaning build directories...'
sleep .5

rm -rf ./build
rm -rf ./bin

source ./scripts/show/divbar
echo '2- Compiling (Debug)...'
sleep .5

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
cd ..

mkdir -p bin

source ./scripts/show/divbar
echo '3- Copying binary and configs...'
sleep .5

cp ./build/retroserver ./bin/retroserver
cp -r ./configs ./bin/configs

# Copy ssl dir if it exists
if [ -d "./ssl" ]; then
    cp -r ./ssl ./bin/ssl
fi


source ./scripts/show/divbar
echo '4- Cleaning build directory...'
sleep .5

rm -rf ./build

source ./scripts/show/divbar
echo 'Done!!'
sleep .5
