#!/bin/bash

# Exit on error
set -e

echo "Cleaning build directory..."
rm -rf build
mkdir -p build

echo "Configuring project with static V8 library..."
cd build
cmake ..

echo "Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

echo "Build complete. The executable is located at: $(pwd)/bin/tiny_node"
echo "You can run it with: ./bin/tiny_node <javascript_file>" 