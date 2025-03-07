#!/bin/bash

# Find V8 include directory
V8_INCLUDE_DIR=$(find /usr /usr/local /opt/local /opt/homebrew -name "v8.h" -type f 2>/dev/null | head -n 1 | xargs dirname)

if [ -z "$V8_INCLUDE_DIR" ]; then
    echo "Error: Could not find V8 include directory"
    exit 1
fi

echo "Found V8 include directory: $V8_INCLUDE_DIR"

# Find V8 library
V8_LIB_DIR=$(find /usr /usr/local /opt/local /opt/homebrew -name "libv8*.so" -o -name "libv8*.dylib" 2>/dev/null | head -n 1 | xargs dirname)

if [ -z "$V8_LIB_DIR" ]; then
    echo "Error: Could not find V8 library directory"
    exit 1
fi

echo "Found V8 library directory: $V8_LIB_DIR"

# Compile the check program
g++ -I"$V8_INCLUDE_DIR" -L"$V8_LIB_DIR" -o check_v8_version check_v8_version.cpp -lv8

# Make the script executable
chmod +x check_v8_version

echo "Build complete. Run ./check_v8_version to check the V8 version." 