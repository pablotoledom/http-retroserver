#!/bin/bash
source ./scripts/show/welcome

source ./scripts/show/divbar
echo '1- Compiling for production...'
sleep .5
./scripts/compile.sh

source ./scripts/show/divbar
echo '2- Installing binary and configs...'
sleep .5

INSTALL_DIR="/usr/local/bin/retroserver"
SERVE_DIR="/srv/retroserver"

mkdir -p "$INSTALL_DIR"
mkdir -p "$SERVE_DIR"

cp ./bin/retroserver "$INSTALL_DIR/retroserver"
cp -r ./configs "$INSTALL_DIR/configs"

if [ -d "./ssl" ]; then
    cp -r ./ssl "$INSTALL_DIR/ssl"
fi

source ./scripts/show/divbar
echo '3- Installing systemd service...'
sleep .5

cp ./scripts/retroserver.service /etc/systemd/system/

systemctl daemon-reload
systemctl enable retroserver.service
systemctl start retroserver.service

source ./scripts/show/divbar
echo "Installed!"
echo "  Binary: $INSTALL_DIR/retroserver"
echo "  Serve dir: $SERVE_DIR  (put your files here)"
echo "  Service: systemctl status retroserver"
sleep .5
