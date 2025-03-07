#!/bin/bash

# Script to run tests from the test directory
# Usage: ./bin/run_test.sh <test_file>
# Example: ./bin/run_test.sh simple_test.js

# Exit on error
set -e

# Check if a test file was provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_file>"
    echo "Available test files:"
    ls -1 test/
    exit 1
fi

TEST_FILE="$1"

# Add test/ prefix if not already present
if [[ $TEST_FILE != test/* ]]; then
    TEST_FILE="test/$TEST_FILE"
fi

# Check if the test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo "Error: Test file '$TEST_FILE' not found."
    echo "Available test files:"
    ls -1 test/
    exit 1
fi

# Check if the executable exists
if [ ! -f "build/bin/tiny_node" ]; then
    echo "Error: Executable not found. Building the project first..."
    ./bin/rebuild_with_static_v8.sh
fi

# Run the test
echo "Running test: $TEST_FILE"
./build/bin/tiny_node $TEST_FILE "$@" 