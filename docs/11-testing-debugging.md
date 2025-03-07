# 11. Testing and Debugging

Testing and debugging are essential aspects of developing a JavaScript runtime. This chapter explores how to test and debug Tiny Node.js, including approaches for testing its functionality, identifying and fixing issues, and ensuring the reliability of the runtime.

## The Importance of Testing a JavaScript Runtime

Testing a JavaScript runtime is crucial for several reasons:

1. **Reliability**: A JavaScript runtime must reliably execute code as expected
2. **Compatibility**: It should behave consistently with established standards
3. **Performance**: It should execute code efficiently
4. **Safety**: It should handle errors gracefully without crashing
5. **Memory Management**: It should properly manage memory to avoid leaks

Testing helps ensure that the runtime meets these requirements and continues to do so as changes are made.

## Types of Tests for a JavaScript Runtime

Several types of tests are useful for a JavaScript runtime:

1. **Unit Tests**: Testing individual components in isolation
2. **Integration Tests**: Testing how components work together
3. **Functional Tests**: Testing the runtime's behavior from a user's perspective
4. **Conformance Tests**: Testing compliance with JavaScript standards
5. **Performance Tests**: Measuring execution speed and memory usage
6. **Stress Tests**: Testing the runtime under heavy load or challenging conditions

In Tiny Node.js, we focus primarily on functional tests, as they provide the most value with the least complexity.

## Test Files in Tiny Node.js

Tiny Node.js includes several test files that verify different aspects of its functionality:

1. **simple_test.js**: A basic test that verifies JavaScript execution
2. **test.js**: A comprehensive test that verifies all core functionality
3. **process_test.js**: Tests specific to the process module
4. **http_test.js**: Tests specific to the HTTP module
5. **math.js**: A module used by test files to verify module loading

Let's examine each of these test files and what they verify.

### simple_test.js

This file performs a simple "Hello, world!" test to verify basic JavaScript execution:

```javascript
// Simple test to verify basic JavaScript execution
print("Hello, world!");
```

This test verifies:
- The runtime can initialize V8
- JavaScript code can be executed
- The `print` function works

It's useful as a quick sanity check that the runtime is working at all.

### test.js

This is the main test file that verifies all core functionality:

```javascript
// Main test file for Tiny Node.js
print("Hello from Tiny Node.js!");

// Test setTimeout
print("Testing setTimeout...");
setTimeout(() => {
    print("Timeout executed after 1000ms");
}, 1000);

// Test module loading
print("Loading math module...");
const math = require('./math');
print("2 + 3 = " + math.add(2, 3));
print("5 - 2 = " + math.subtract(5, 2));
print("4 * 3 = " + math.multiply(4, 3));
print("10 / 2 = " + math.divide(10, 2));

// Test fs module
print("Loading fs module...");
const fs = require('fs');
print("Does math.js exist? " + fs.exists('math.js'));
print("Does nonexistent.js exist? " + fs.exists('nonexistent.js'));

print("Reading math.js:");
const mathSource = fs.readFile('math.js');
print(mathSource);

print("Writing to test-output.txt");
fs.writeFile('test-output.txt', 'This is a test file created by Tiny Node.js');
print("File written successfully");

print("Reading test-output.txt:");
const testOutput = fs.readFile('test-output.txt');
print(testOutput);

// Test http module
print("Loading http module...");
const http = require('http');
print("Creating HTTP server...");
const server = http.createServer((req, res) => {
    print("Received request: " + req.method + " " + req.url);
    res.statusCode = 200;
    res.setHeader('Content-Type', 'text/plain');
    res.end('Hello from Tiny Node.js HTTP Server!');
    print("Response status: " + res.statusCode);
    print("Response body: Hello from Tiny Node.js HTTP Server!");
});

print("Starting HTTP server on port 8080...");
server.listen(8080, () => {
    print("HTTP server is running on http://localhost:8080/");
});

// Test process module
print("Testing process module...");
print("Node.js version: " + process.version);
print("Platform: " + process.platform);
print("Architecture: " + process.arch);
print("Current directory: " + process.cwd());
print("Command line args: " + process.argv.join(" "));
print("USER env variable: " + process.env.USER);
print("Process module test completed successfully");

// Return a value to verify that the script executed fully
3;
```

This test verifies:
- Basic JavaScript execution
- The `setTimeout` function
- Module loading with `require`
- The `fs` module's `exists`, `readFile`, and `writeFile` functions
- The `http` module's server creation and request handling
- The `process` module's properties and methods

It's a comprehensive test that ensures all core functionality is working correctly.

### process_test.js

This file specifically tests the process module:

```javascript
// Test file for the process module
print("===== Process Module Test =====");

// Test process.argv
print("Testing process.argv:");
print("Command-line arguments:");
process.argv.forEach((arg, index) => {
    print(`  argv[${index}] = ${arg}`);
});

// Test process.env
print("\nTesting process.env:");
print("Environment variables (partial list):");
print(`  PATH = ${process.env.PATH}`);
print(`  HOME = ${process.env.HOME}`);
print(`  USER = ${process.env.USER}`);

// Test process.cwd()
print("\nTesting process.cwd():");
print(`Current working directory: ${process.cwd()}`);

// Test process properties
print("\nTesting process properties:");
print(`Type of process: ${typeof process}`);
print("Available process properties:");
Object.keys(process).forEach(key => {
    print(`  ${key} (${typeof process[key]})`);
});

// Test process.exit (commented out to avoid terminating the test)
print("\nProcess exit test is skipped to avoid terminating the test.");
// process.exit(0);

print("\n===== Process Module Test Complete =====");
```

This test verifies:
- The `process.argv` array contains the expected command-line arguments
- The `process.env` object contains environment variables
- The `process.cwd()` method returns the current working directory
- The process object has the expected properties and methods

It's a focused test that ensures the process module is functioning correctly.

### http_test.js

This file specifically tests the HTTP module:

```javascript
// Test file for the HTTP module
const http = require('http');

// Create an HTTP server
const server = http.createServer((req, res) => {
    print("Received " + req.method + " request for " + req.url);
    
    res.statusCode = 200;
    res.setHeader('Content-Type', 'text/plain');
    res.end('Hello from Tiny Node.js HTTP Server!');
    
    print("Response status: " + res.statusCode);
    print("Response body: Hello from Tiny Node.js HTTP Server!");
});

// Start the server
server.listen(3000, () => {
    print("Server running at http://localhost:3000/");
});

// Return 1 to indicate success
1;
```

This test verifies:
- The `http` module can be loaded
- An HTTP server can be created
- The server can listen on a port
- Request and response objects work as expected

It's a focused test that ensures the HTTP module is functioning correctly.

### math.js

This file is a module used by other test files to verify module loading:

```javascript
// Math module

// Add two numbers
exports.add = function(a, b) {
    return a + b;
};

// Subtract two numbers
exports.subtract = function(a, b) {
    return a - b;
};

// Multiply two numbers
exports.multiply = function(a, b) {
    return a * b;
};

// Divide two numbers
exports.divide = function(a, b) {
    if (b === 0) {
        throw new Error("Division by zero");
    }
    return a / b;
};

// Print a message when the module is loaded
print("Math module loaded");
```

This module:
- Exports several math functions
- Prints a message when loaded
- Includes error handling (division by zero)

It's used to verify that module loading and exports work correctly.

## Running Tests

Tiny Node.js provides several ways to run tests:

1. **Running a Single Test**:
```bash
./bin/run_test.sh simple_test.js
```

2. **Running All Tests**:
```bash
./bin/run_all_tests.sh
```

3. **Running a Test Directly**:
```bash
./build/bin/tiny_node test/test.js
```

The test scripts include detailed logging to help identify issues:

```
Starting main function...
Initializing runtime...
Initializing V8...
Platform created, initializing V8 platform...
V8 platform initialized, initializing V8...
V8 initialized successfully
Creating runtime instance...
Runtime constructor: Creating isolate...
Runtime constructor: Isolate created
Runtime constructor: Creating global template...
Runtime constructor: Creating module system...
Runtime constructor: Setting up global functions...
RegisterNativeFunction: Starting for print...
RegisterNativeFunction: Storing function for later use
RegisterNativeFunction: Complete for print
RegisterNativeFunction: Starting for setTimeout...
RegisterNativeFunction: Storing function for later use
RegisterNativeFunction: Complete for setTimeout
RegisterNativeFunction: Starting for clearTimeout...
RegisterNativeFunction: Storing function for later use
RegisterNativeFunction: Complete for clearTimeout
RegisterNativeFunction: Starting for require...
RegisterNativeFunction: Storing function for later use
RegisterNativeFunction: Complete for require
Runtime constructor: Registering native modules...
RegisterNativeModules: Starting...
RegisterNativeModules: Registering native modules...
RegisterNativeModules: Registering fs module...
RegisterFsModule: Starting...
RegisterFsModule: Got isolate
RegisterFsModule: Created handle scope
RegisterFsModule: Created and entered context
RegisterFsModule: Created fs object
RegisterFsModule: Adding readFile function...
RegisterFsModule: Adding writeFile function...
RegisterFsModule: Adding exists function...
RegisterFsModule: Registering module with ModuleSystem...
RegisterFsModule: Complete
RegisterNativeModules: Registering http module...
RegisterHttpModule: Starting...
RegisterHttpModule: Got isolate
RegisterHttpModule: Created handle scope
RegisterHttpModule: Created and entered context
RegisterHttpModule: Created http object
RegisterHttpModule: Adding createServer function...
RegisterHttpModule: Registering module with ModuleSystem...
RegisterHttpModule: Complete
RegisterNativeModules: Complete
Runtime constructor: Creating event loop...
Runtime constructor: Complete
...
```

This detailed logging helps identify where issues occur, making it easier to debug.

## Debugging Techniques

Several techniques are useful for debugging Tiny Node.js:

### 1. Console Logging

Console logging is the simplest debugging technique. In C++ code, use `std::cout` or `std::cerr`:

```cpp
std::cout << "Debug: " << value << std::endl;
```

In JavaScript code, use the `print` function:

```javascript
print("Debug: " + value);
```

Tiny Node.js includes extensive logging throughout its codebase to make it easier to understand what's happening and identify issues.

### 2. Using a Debugger

For more complex issues, using a debugger like GDB or LLDB can be helpful:

```bash
# Start the runtime with GDB
gdb --args ./build/bin/tiny_node test/test.js

# Set a breakpoint
(gdb) break Runtime::ExecuteFile

# Run the program
(gdb) run

# Step through the code
(gdb) step

# Examine variables
(gdb) print isolate
```

This allows for step-by-step debugging, examining variables, and understanding the flow of execution.

### 3. Memory Debugging

For memory issues, tools like Valgrind can help identify leaks:

```bash
valgrind --leak-check=full ./build/bin/tiny_node test/test.js
```

This will report any memory leaks or access errors.

### 4. V8 Debugging Flags

V8 provides various debugging flags that can help identify issues:

```bash
# Enable GC traces
./build/bin/tiny_node --trace-gc test/test.js

# Enable compiler traces
./build/bin/tiny_node --trace-compiler test/test.js
```

These flags can provide insight into V8's internal behavior.

### 5. JavaScript Error Handling

Proper JavaScript error handling is essential for identifying issues in JavaScript code:

```javascript
try {
    // Code that might throw
    const result = riskyOperation();
    print("Result: " + result);
} catch (error) {
    print("Error: " + error.message);
}
```

This allows for graceful handling of errors and provides useful debugging information.

## Common Issues and Solutions

Several common issues can arise when developing a JavaScript runtime:

### 1. Memory Leaks

**Issue**: The runtime consumes more memory over time.

**Solution**:
- Ensure all V8 handles are properly managed
- Use handle scopes to manage local handles
- Reset and delete persistent handles when no longer needed
- Clean up LibUV handles and requests

### 2. Segmentation Faults

**Issue**: The runtime crashes with a segmentation fault.

**Solution**:
- Check for null pointers
- Ensure V8 handles are not used after their scope ends
- Verify that the isolate is valid
- Check for issues with callback functions

### 3. JavaScript Exceptions

**Issue**: JavaScript code throws exceptions.

**Solution**:
- Use V8's `TryCatch` to catch and handle exceptions
- Add error checking to native functions
- Provide meaningful error messages

### 4. Event Loop Issues

**Issue**: Asynchronous operations don't complete or the event loop doesn't exit.

**Solution**:
- Check that all LibUV handles are properly closed
- Ensure callbacks are correctly registered
- Verify that the event loop is running correctly

### 5. Module Loading Issues

**Issue**: Modules fail to load or their exports aren't accessible.

**Solution**:
- Check the module path resolution
- Verify that the module file exists
- Ensure the module's exports are correctly set
- Check for circular dependencies

## Performance Testing

Performance testing is important for ensuring the runtime is efficient. Here are some approaches:

### 1. Benchmark Scripts

Create scripts that measure the performance of specific operations:

```javascript
// Measure execution time
const start = Date.now();
for (let i = 0; i < 1000000; i++) {
    // Operation to benchmark
    Math.sqrt(i);
}
const end = Date.now();
print(`Execution time: ${end - start} ms`);
```

### 2. Memory Usage Tracking

Track memory usage to identify leaks or inefficiencies:

```cpp
// Print memory usage
size_t heap_size = isolate->GetHeapStatistics().total_heap_size();
std::cout << "Heap size: " << heap_size << " bytes" << std::endl;
```

### 3. Profiling

Use V8's built-in profiler to identify performance bottlenecks:

```bash
# Run with profiling enabled
./build/bin/tiny_node --prof test/test.js

# Process the profiling data
./tools/process_prof.js v8.log > processed.txt
```

This can help identify where time is being spent in the runtime.

## Testing Across Platforms

A JavaScript runtime should work consistently across different platforms. To ensure this:

1. **Compile and test on different operating systems**:
   - Windows
   - macOS
   - Linux

2. **Test on different architectures**:
   - x86_64
   - ARM64
   - i386

3. **Account for platform differences**:
   - File paths (backslashes vs. forward slashes)
   - Environment variables
   - System calls

## Automated Testing

For a more robust testing process, automated testing can be implemented:

1. **Continuous Integration (CI)**:
   - Set up a CI system (e.g., GitHub Actions, Travis CI)
   - Run tests automatically on each commit
   - Build on multiple platforms

2. **Test Framework**:
   - Implement a simple test framework
   - Define test cases and expected results
   - Automatically run and verify tests

3. **Coverage Analysis**:
   - Measure code coverage to identify untested areas
   - Focus testing efforts on areas with low coverage

## Conclusion

Testing and debugging are crucial aspects of developing a JavaScript runtime. By implementing comprehensive tests, using effective debugging techniques, and addressing common issues, you can ensure that your runtime is reliable, efficient, and behaves as expected.

The approaches described in this chapter can help identify and resolve issues in Tiny Node.js, ensuring that it provides a solid foundation for JavaScript applications.

[← Previous: Process Module](10-process-module.md) | [Next: Advanced Topics →](12-advanced-topics.md) 