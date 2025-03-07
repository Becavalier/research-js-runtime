# Tiny Node.js

A minimal JavaScript runtime inspired by Node.js.

## Project Overview

This project aims to build a lightweight JavaScript runtime from scratch, implementing the core functionality of Node.js:

- JavaScript execution using the V8 engine.
- Event loop for asynchronous operations.
- Basic I/O operations.
- Simple module system.

## Roadmap

### Phase 1: Setup and JavaScript Engine Integration.
- [x] Create project structure.
- [x] Set up build system (CMake).
- [x] Integrate V8 JavaScript engine.
- [x] Execute simple JavaScript code.

### Phase 2: Event Loop and Basic I/O.
- [x] Implement a simple event loop.
- [x] Add file system operations.
- [x] Implement timers (setTimeout, setInterval).

### Phase 3: Module System.
- [x] Create a basic module loading system (similar to CommonJS).
- [x] Implement core modules.
  - [x] fs module (file system operations).
  - [x] http module (simple HTTP server).
  - [x] process module (command-line arguments, environment variables).

### Phase 4: Refinement and Extensions.
- [x] Add error handling.
- [ ] Implement more Node.js APIs.
- [ ] Performance optimization.

## Project Structure

```
research-js-runtime/
│
├── bin/                   # Shell scripts for building and testing
│   ├── build_and_run.sh       # Build the project and run a script
│   ├── build_check_v8.sh      # Check V8 compatibility and build
│   ├── rebuild_with_static_v8.sh # Rebuild with static V8 library
│   ├── run_test.sh            # Convenience script for running a test file
│   ├── run_all_tests.sh       # Run all tests in the test directory
│   └── run_example.sh         # Run JavaScript code snippets directly
│
├── build/                 # Build output directory
│
├── examples/              # Example JavaScript programs
│   ├── hello.js              # Basic example demonstrating core features
│   └── server.js             # HTTP server example
│
├── include/               # Header files
│
├── src/                   # Source files
│
├── test/                  # Test files
│   ├── test.js               # Main test script
│   ├── simple_test.js        # Basic JavaScript execution test
│   ├── http_test.js          # HTTP server test
│   ├── process_test.js       # Process module test
│   ├── math.js               # Math module for testing
│   └── test-output.txt       # Test output file
│
└── README.md              # This file
```

## Building and Running

### Prerequisites

- CMake 3.10 or higher.
- C++20 compatible compiler.
- V8 JavaScript engine.
- libuv (for the event loop).

### Building

```bash
# Build with the convenience script
./bin/rebuild_with_static_v8.sh

# Or manually with CMake
mkdir -p build
cd build
cmake ..
make
```

### Running

```bash
# Run a JavaScript file directly
./build/bin/tiny_node path/to/script.js

# Or use the convenience scripts
./bin/run_test.sh simple_test.js    # Runs test/simple_test.js
./bin/run_all_tests.sh              # Runs all test files
./bin/run_example.sh "print('Hello, world!');"  # Run code directly
./bin/run_example.sh -e "2 + 2"     # Evaluate an expression

# Run examples
./build/bin/tiny_node examples/hello.js
./build/bin/tiny_node examples/server.js
```

## Convenience Scripts

The project includes several shell scripts to make development and testing easier:

- `bin/rebuild_with_static_v8.sh` - Rebuild the project with static V8 library
- `bin/run_test.sh` - Run a specific test file from the test directory
- `bin/run_all_tests.sh` - Run all test files and report results
- `bin/run_example.sh` - Run JavaScript code snippets without creating a file

The example script has several options:
```bash
# Show help
./bin/run_example.sh --help

# Run code directly
./bin/run_example.sh "print('Hello, world!');"

# Evaluate an expression (automatically adds print())
./bin/run_example.sh -e "2 + 2"

# Run a specific file
./bin/run_example.sh -f path/to/file.js
```

## Examples

The `examples/` directory contains demonstration scripts:

- `hello.js` - Basic example demonstrating printing, setTimeout, and process information
- `server.js` - HTTP server example with different endpoints and content types

To run these examples:

```bash
# Basic hello world example
./build/bin/tiny_node examples/hello.js

# HTTP server example (then open http://localhost:3000 in your browser)
./build/bin/tiny_node examples/server.js
```

## Test Scripts

The `test/` directory contains various test scripts:

- `test.js` - Comprehensive test script demonstrating all functionality
- `simple_test.js` - Basic test for JavaScript execution
- `http_test.js` - Test for the HTTP server module
- `process_test.js` - Test for the process module
- `math.js` - Module with math functions used by other tests

