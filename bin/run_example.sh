#!/bin/bash

# Script to run JavaScript code examples directly from the command line
# Usage: ./bin/run_example.sh "console.log('Hello, world!');"
# Or:    ./bin/run_example.sh -e "2 + 2"
# Or:    ./bin/run_example.sh -f "path/to/example.js"

# Exit on error
set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Display usage information
show_usage() {
    echo -e "${BLUE}Usage:${NC}"
    echo -e "  $0 [options] <code_or_file>"
    echo
    echo -e "${BLUE}Options:${NC}"
    echo "  -h, --help       Show this help message"
    echo "  -e, --eval       Evaluate the given JavaScript code"
    echo "  -f, --file       Run the specified JavaScript file"
    echo
    echo -e "${BLUE}Examples:${NC}"
    echo "  $0 \"print('Hello, world!');\""
    echo "  $0 -e \"2 + 2\""
    echo "  $0 -f \"examples/hello.js\""
    echo
}

# Check if executable exists and rebuild if necessary
check_executable() {
    if [ ! -f "build/bin/tiny_node" ]; then
        echo "Executable not found. Building the project first..."
        ./bin/rebuild_with_static_v8.sh
    fi
}

# Run code provided as a string
run_code() {
    local code="$1"
    local temp_file="$(mktemp)"
    
    # Add print statement if the code appears to be an expression
    if [[ ! "$code" =~ .*\;.* ]]; then
        code="print($code);"
    fi
    
    echo "$code" > "$temp_file"
    echo -e "${GREEN}Running code:${NC} $code"
    ./build/bin/tiny_node "$temp_file"
    rm "$temp_file"
}

# Run code from a file
run_file() {
    local file="$1"
    
    if [ ! -f "$file" ]; then
        echo "Error: File not found: $file"
        exit 1
    fi
    
    echo -e "${GREEN}Running file:${NC} $file"
    ./build/bin/tiny_node "$file"
}

# Process command-line arguments
if [ $# -eq 0 ]; then
    show_usage
    exit 0
fi

# Check if we have the executable
check_executable

# Parse options
case "$1" in
    -h|--help)
        show_usage
        exit 0
        ;;
    -e|--eval)
        if [ -z "$2" ]; then
            echo "Error: No code provided to evaluate"
            exit 1
        fi
        run_code "$2"
        ;;
    -f|--file)
        if [ -z "$2" ]; then
            echo "Error: No file provided to run"
            exit 1
        fi
        run_file "$2"
        ;;
    *)
        # Default behavior: treat the first argument as code to run
        run_code "$1"
        ;;
esac

exit 0 