#!/bin/bash

which pkg-config &>/dev/null
if [ $? -ne 0 ]; then
    echo "===== Oops! Command \"pkg-config\" not found. ====="
    echo "  -> Please install it first!"
    echo "  -> This is usually done by something like \"sudo apt install pkg-config\""
    echo
    exit 1
fi

zlib_dir=$(pkg-config --variable=libdir zlib 2>/dev/null)
if [ $? -ne 0 ]; then
    echo "===== Oops! No library libz.so can be found by pkg-config. ====="
    exit 1
fi

sed "s#ZLIB_PATH#${zlib_dir}#g" WORKSPACE.in > WORKSPACE

udev_dir=$(pkg-config --variable=libdir libudev 2>/dev/null)
if [ $? -ne 0 ]; then
    echo "===== Oops! No libudev found by pkg-config. ====="
    exit 1
fi
sed "s#UDEV_PATH#${udev_dir}#g" WORKSPACE.in > WORKSPACE


