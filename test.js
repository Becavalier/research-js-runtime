/**
 * Test Script for Tiny Node.js Runtime
 * 
 * This script tests various features of our tiny Node.js implementation:
 * - Basic JavaScript execution
 * - setTimeout functionality
 * - Module loading (CommonJS-style require)
 * - File system operations
 */

// Print a message to test basic functionality
print("Hello from Tiny Node.js!");

/**
 * Test setTimeout functionality
 * 
 * This demonstrates the event loop and asynchronous execution.
 * The callback function will be executed after approximately 1000ms.
 */
print("Testing setTimeout...");
setTimeout(function() {
    print("Timeout executed after 1000ms");
}, 1000);

/**
 * Test module system
 * 
 * This demonstrates loading a JavaScript module using require().
 * The math.js module exports several math functions that we can use.
 */
try {
    print("Loading math module...");
    const math = require('./math');
    
    // Test the exported functions
    print("2 + 3 =", math.add(2, 3));
    print("5 - 2 =", math.subtract(5, 2));
    print("4 * 3 =", math.multiply(4, 3));
    print("10 / 2 =", math.divide(10, 2));
} catch (e) {
    print("Module loading failed:", e.message);
}

/**
 * Test file system (fs) module
 * 
 * This demonstrates using a native module implemented in C++.
 * The fs module provides basic file system operations.
 */
try {
    print("Loading fs module...");
    const fs = require('fs');
    
    // Test exists function
    print("Does math.js exist?", fs.exists("math.js"));
    print("Does nonexistent.js exist?", fs.exists("nonexistent.js"));
    
    // Test readFile function
    print("Reading math.js:");
    const mathContent = fs.readFile("math.js");
    print(mathContent);
    
    // Test writeFile function
    print("Writing to test-output.txt");
    fs.writeFile("test-output.txt", "This is a test file created by Tiny Node.js");
    print("File written successfully");
    
    // Read the file back to verify
    print("Reading test-output.txt:");
    const testContent = fs.readFile("test-output.txt");
    print(testContent);
} catch (e) {
    print("FS module test failed:", e.message);
}

// Keep the runtime alive for a bit to allow async operations to complete
// In a real Node.js application, the process would exit when all tasks are done
setTimeout(function() {
    print("Exiting...");
}, 1500); 