#!/bin/sh
set -xe

# Check if clang is available
if command -v clang > /dev/null 2>&1; then
    C_COMPILER=clang
# Check if gcc is available
elif command -v gcc > /dev/null 2>&1; then
    C_COMPILER=gcc
# Default to cc if neither clang nor gcc is available
else
    C_COMPILER=cc
fi

$C_COMPILER -o bundle bundle.c

