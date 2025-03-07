#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building Tiny Node.js ===${NC}"

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. || { echo -e "${RED}CMake configuration failed${NC}"; exit 1; }

# Build the project
echo -e "${YELLOW}Building the project...${NC}"
cmake --build . || { echo -e "${RED}Build failed${NC}"; exit 1; }

# Return to the root directory
cd ..

# Check if the executable was created
if [ ! -f "build/bin/tiny_node" ]; then
    echo -e "${RED}Build failed: executable not found${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Run the tests if requested
if [ "$1" = "test" ] || [ "$1" = "all" ]; then
    echo -e "${GREEN}=== Running Tests ===${NC}"
    
    echo -e "${YELLOW}Running basic test...${NC}"
    build/bin/tiny_node test.js
    
    if [ "$1" = "all" ]; then
        echo -e "${YELLOW}Running HTTP server test...${NC}"
        build/bin/tiny_node http_test.js &
        HTTP_PID=$!
        echo "HTTP server started with PID $HTTP_PID"
        echo "You can test it by opening http://localhost:3000/ in your browser"
        echo "Press Ctrl+C to stop the HTTP server"
        
        # Wait for user to press Ctrl+C
        trap "kill $HTTP_PID; echo -e '${YELLOW}HTTP server stopped${NC}'" INT
        wait $HTTP_PID
    fi
fi

echo -e "${GREEN}=== Done ===${NC}"
echo "You can run the tiny Node.js runtime with: build/bin/tiny_node <js-file>" 