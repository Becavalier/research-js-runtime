#!/bin/bash

# Script to run all tests from the test directory
# Usage: ./bin/run_all_tests.sh

# Exit on error
set -e

# Check if the executable exists
if [ ! -f "build/bin/tiny_node" ]; then
    echo "Error: Executable not found. Building the project first..."
    ./bin/rebuild_with_static_v8.sh
fi

# Define ANSI color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to run a test and check its exit code
run_test() {
    local test_file="$1"
    echo -e "\n${GREEN}===============================================${NC}"
    echo -e "${GREEN}Running test: $test_file${NC}"
    echo -e "${GREEN}===============================================${NC}"
    
    if ./build/bin/tiny_node "test/$test_file"; then
        echo -e "${GREEN}✓ Test $test_file passed${NC}"
        return 0
    else
        echo -e "${RED}✗ Test $test_file failed${NC}"
        return 1
    fi
}

# Get list of all JS files in the test directory, excluding modules like math.js
TEST_FILES=$(find test -name "*.js" -type f -not -name "math.js" -exec basename {} \;)

# Initialize counters
TOTAL=0
PASSED=0
FAILED=0

# Run each test
for test_file in $TEST_FILES; do
    ((TOTAL++))
    if run_test "$test_file"; then
        ((PASSED++))
    else
        ((FAILED++))
        FAILED_TESTS="$FAILED_TESTS $test_file"
    fi
done

# Print summary
echo -e "\n${GREEN}===============================================${NC}"
echo -e "${GREEN}Test Results:${NC}"
echo -e "${GREEN}===============================================${NC}"
echo -e "Total tests: $TOTAL"
echo -e "Passed: ${GREEN}$PASSED${NC}"

if [ $FAILED -gt 0 ]; then
    echo -e "Failed: ${RED}$FAILED${NC}"
    echo -e "Failed tests:${RED}$FAILED_TESTS${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi 