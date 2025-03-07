# 3. Project Structure

Understanding the organization of Tiny Node.js will help you navigate the codebase and make changes. This chapter explains the project's directory structure, key components, and build system.

## Directory Organization

Tiny Node.js follows a clean, logical directory structure:

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
└── README.md              # Project documentation
```

Let's explore each of these directories and their contents:

### `bin/` - Utility Scripts

The `bin/` directory contains shell scripts that simplify building, testing, and running the project:

- `rebuild_with_static_v8.sh`: Cleans and rebuilds the project with the static V8 library
- `build_check_v8.sh`: Verifies V8 compatibility and builds the project
- `build_and_run.sh`: Builds the project and runs a specified JavaScript file
- `run_test.sh`: Runs a specific test file from the test directory
- `run_all_tests.sh`: Runs all test files and reports the results
- `run_example.sh`: Allows running JavaScript code snippets directly from the command line

These scripts make it easier to work with the project, especially for common tasks like rebuilding and testing.

### `include/` - Header Files

The `include/` directory contains C++ header files that define the public interfaces of our classes and functions:

- `runtime.h`: Declares the Runtime class that manages the JavaScript execution environment
- `module_system.h`: Defines the ModuleSystem class for handling module loading
- `fs_module.h`: Declares functions for the file system module
- `http_module.h`: Contains declarations for the HTTP server module
- `process_module.h`: Defines functionality for accessing process information

### `src/` - Source Files

The `src/` directory contains the implementation files corresponding to the headers in `include/`:

- `main.cpp`: Entry point of the application
- `runtime.cpp`: Implementation of the Runtime class
- `module_system.cpp`: Implementation of the module loading system
- `fs_module.cpp`: File system module implementation
- `http_module.cpp`: HTTP server implementation
- `process_module.cpp`: Process information module implementation

### `test/` - Test Files

The `test/` directory contains JavaScript test files that verify the functionality of Tiny Node.js:

- `test.js`: Comprehensive test covering all core features
- `simple_test.js`: Basic JavaScript execution test
- `http_test.js`: Tests the HTTP server functionality
- `process_test.js`: Verifies process module features
- `math.js`: A JavaScript module used by other test files

### `examples/` - Example Programs

The `examples/` directory contains sample JavaScript programs that demonstrate how to use Tiny Node.js:

- `hello.js`: A simple example showing basic functionality like printing and process info
- `server.js`: An HTTP server example showing how to create a web server

### `build/` - Build Output

The `build/` directory is created by the build system and contains the compiled binaries and intermediate files. This directory is typically excluded from version control.

## Key Components

The major components of Tiny Node.js include:

### 1. Runtime

The Runtime class (`include/runtime.h` and `src/runtime.cpp`) is the core of Tiny Node.js. It:

- Initializes and manages the V8 engine
- Creates the JavaScript execution environment
- Registers native functions and modules
- Handles execution of JavaScript files
- Manages the event loop
- Provides cleanup and shutdown functionality

### 2. Module System

The ModuleSystem class (`include/module_system.h` and `src/module_system.cpp`) implements a simplified version of Node.js's CommonJS module system. It:

- Handles `require()` calls from JavaScript
- Loads and caches JavaScript modules
- Registers native modules (like `fs` and `http`)
- Maintains module paths and resolution

### 3. Native Modules

Native modules provide JavaScript interfaces to system functionality:

- **FS Module** (`include/fs_module.h` and `src/fs_module.cpp`): File system operations
- **HTTP Module** (`include/http_module.h` and `src/http_module.cpp`): HTTP server functionality
- **Process Module** (`include/process_module.h` and `src/process_module.cpp`): Process information and control

### 4. Native Functions

Native functions are C++ implementations of JavaScript functions that are globally available:

- `print()`: Outputs text to the console
- `setTimeout()`: Schedules a function to run after a delay
- `clearTimeout()`: Cancels a scheduled timeout
- `require()`: Loads and returns a module

### 5. Main Entry Point

The main entry point (`src/main.cpp`) initializes the runtime, processes command-line arguments, and executes the specified JavaScript file.

## Build System

Tiny Node.js uses CMake as its build system, which provides cross-platform build capabilities.

### CMake Configuration

The root `CMakeLists.txt` file defines the project structure, dependencies, and build targets:

```cmake
cmake_minimum_required(VERSION 3.10)
project(TinyNode VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(V8 REQUIRED)
find_package(LibUV REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${V8_INCLUDE_DIR}
    ${LIBUV_INCLUDE_DIRS}
)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Executable
add_executable(tiny_node ${SOURCES})

# Link libraries
target_link_libraries(tiny_node
    ${V8_LIBRARIES}
    ${LIBUV_LIBRARIES}
)
```

### Building the Project

You can build the project using the provided scripts:

```bash
# Using the convenience script
./bin/rebuild_with_static_v8.sh

# Or manually with CMake
mkdir -p build
cd build
cmake ..
make
```

### Dependencies

Tiny Node.js has two main external dependencies:

1. **V8**: The JavaScript engine
2. **LibUV**: The library that provides the event loop and asynchronous I/O

These dependencies must be properly installed and configured for the build to succeed.

## Understanding the Workflow

When you run a JavaScript file with Tiny Node.js, the following sequence occurs:

1. The `main.cpp` entry point initializes the runtime
2. The Runtime class sets up V8 and creates the execution environment
3. Native functions and modules are registered
4. The specified JavaScript file is loaded and executed
5. The event loop processes asynchronous operations
6. When the event loop is empty, the runtime shuts down

This structure allows for a clean separation of concerns while maintaining a simple, understandable codebase.

## Navigation Tips

Here are some tips for navigating the codebase:

1. Start with `main.cpp` to understand the entry point and initialization
2. Examine `runtime.h` and `runtime.cpp` to understand the core functionality
3. Look at `module_system.h` and `module_system.cpp` to understand module loading
4. Explore the native modules to see how C++ functionality is exposed to JavaScript
5. Read the test files to understand how the runtime is expected to behave

By understanding the project structure, you'll be better equipped to explore, modify, and extend Tiny Node.js.

[← Previous: V8 JavaScript Engine Basics](02-v8-basics.md) | [Next: Runtime Core Implementation →](04-runtime-core.md) 