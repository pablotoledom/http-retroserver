#!/bin/bash
mkdir -p ./ssl

openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout ./ssl/key.pem -out ./ssl/cert.pem \
    -subj "/C=US/ST=State/L=City/O=retroserver/CN=localhost"

echo ''
echo "Certificate created:"
echo "  ./ssl/cert.pem"
echo "  ./ssl/key.pem"
echo ''
echo "Enable HTTPS in configs/config.txt:"
echo "  ssl_enabled=1"
echo "  ssl_cert=./ssl/cert.pem"
echo "  ssl_key=./ssl/key.pem"
