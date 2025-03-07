# Troubleshooting Guide

This document provides solutions for common issues you might encounter when building and running the tiny Node.js project.

## Build Issues

### CMake Can't Find V8 or libuv

**Symptoms:**
- CMake configuration fails with errors about missing V8 or libuv
- Error messages like "Could NOT find V8" or "Could NOT find LibUV"

**Solutions:**
1. Make sure V8 and libuv are installed on your system
2. Specify the paths manually:
   ```bash
   cmake .. -DV8_INCLUDE_DIR=/path/to/v8/include -DV8_LIBRARIES=/path/to/v8/lib/libv8.so -DLIBUV_INCLUDE_DIR=/path/to/libuv/include -DLIBUV_LIBRARIES=/path/to/libuv/lib/libuv.so
   ```
3. Set environment variables:
   ```bash
   export V8_DIR=/path/to/v8
   export LIBUV_DIR=/path/to/libuv
   cmake ..
   ```

### Compilation Errors

**Symptoms:**
- Compiler errors about missing headers or undefined symbols
- Errors related to V8 API incompatibility

**Solutions:**
1. Check your V8 version:
   ```bash
   ./build_check_v8.sh
   ./check_v8_version
   ```
2. Adjust the code to match your V8 version (V8 API changes frequently)
3. Make sure you're using C++17 or later:
   ```bash
   cmake .. -DCMAKE_CXX_STANDARD=17
   ```

## Runtime Issues

### JavaScript Execution Errors

**Symptoms:**
- "Failed to execute file" error
- V8 exceptions or segmentation faults

**Solutions:**
1. Check that the JavaScript file exists and is readable
2. Verify that the JavaScript code is compatible with your V8 version
3. Run with a simpler JavaScript file to isolate the issue

### HTTP Server Issues

**Symptoms:**
- HTTP server doesn't start
- Can't connect to the server

**Solutions:**
1. Make sure port 3000 is not already in use:
   ```bash
   lsof -i :3000
   ```
2. Check for firewall issues
3. Try a different port by modifying http_test.js

### Module Loading Issues

**Symptoms:**
- "Module loading failed" errors
- "Cannot find module" errors

**Solutions:**
1. Make sure the module file exists and is in the correct location
2. Check the module path resolution logic in src/module.cpp
3. Verify that the module code is valid JavaScript

## Debugging

For more advanced debugging:

1. Add debug output to the C++ code
2. Use a debugger like GDB or LLDB:
   ```bash
   lldb -- build/bin/tiny_node test.js
   ```
3. Build with debug symbols:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   ``` 