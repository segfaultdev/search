#!/usr/bin/sh

# gcc $(find . -name "*.c") -Iinclude -Ofast -DMG_ENABLE_MBEDTLS=1 -lmbedtls -lmbedcrypto -lmbedx509 -s -o search
gcc $(find . -name "*.c") -Iinclude -Og -DMG_ENABLE_MBEDTLS=1 -lmbedtls -lmbedcrypto -lmbedx509 -g -o search
