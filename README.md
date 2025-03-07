# Tiny Node.js

A minimal JavaScript runtime inspired by Node.js.

## Project Overview

This project aims to build a lightweight JavaScript runtime from scratch, implementing the core functionality of Node.js:

- JavaScript execution using the V8 engine
- Event loop for asynchronous operations
- Basic I/O operations
- Simple module system

## Roadmap

### Phase 1: Setup and JavaScript Engine Integration
- [x] Create project structure
- [x] Set up build system (CMake)
- [x] Integrate V8 JavaScript engine
- [x] Execute simple JavaScript code

### Phase 2: Event Loop and Basic I/O
- [x] Implement a simple event loop
- [x] Add file system operations
- [x] Implement timers (setTimeout, setInterval)

### Phase 3: Module System
- [x] Create a basic module loading system (similar to CommonJS)
- [x] Implement core modules
  - [x] fs module (file system operations)
  - [x] http module (simple HTTP server)
  - [x] process module (command-line arguments, environment variables)

### Phase 4: Refinement and Extensions
- [x] Add error handling
- [ ] Implement more Node.js APIs
- [ ] Performance optimization

## Building and Running

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- V8 JavaScript engine
- libuv (for the event loop)

### Building

```bash
# Create a build directory
mkdir build
cd build

# Configure the build
cmake ..

# Build the project
make
```

### Running

```bash
# Run a JavaScript file
./bin/tiny_node path/to/script.js
```

## Example Scripts

- `test.js` - Basic test script demonstrating JavaScript execution and module loading
- `math.js` - Example module with math functions
- `http_test.js` - Example HTTP server
- `process_test.js` - Example script demonstrating process module functionality

## Dependencies

- V8 JavaScript Engine
- libuv (for the event loop) 