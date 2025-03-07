/**
 * Test Script for Tiny Node.js Runtime
 * 
 * This script tests various features of our tiny Node.js implementation:
 * - Basic JavaScript execution
 * - setTimeout functionality
 * - Module loading (CommonJS-style require)
 * - File system operations
 * - HTTP server functionality
 * - Process information and functionality
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
    const math = require('./test/math');
    
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
    print("Does math.js exist?", fs.exists("test/math.js"));
    print("Does nonexistent.js exist?", fs.exists("nonexistent.js"));
    
    // Test readFile function
    print("Reading math.js:");
    const mathContent = fs.readFile("test/math.js");
    print(mathContent);
    
    // Test writeFile function
    print("Writing to test-output.txt");
    fs.writeFile("test/test-output.txt", "This is a test file created by Tiny Node.js");
    print("File written successfully");
    
    // Read the file back to verify
    print("Reading test-output.txt:");
    const testContent = fs.readFile("test/test-output.txt");
    print(testContent);
} catch (e) {
    print("FS module test failed:", e.message);
}

/**
 * Test HTTP module
 * 
 * This demonstrates using the HTTP module to create a simple web server.
 * Note: This is a simplified mock implementation for testing purposes.
 */
try {
    print("Loading http module...");
    const http = require('http');
    
    // Create an HTTP server
    print("Creating HTTP server...");
    const server = http.createServer(function(req, res) {
        print("Received request:", req.method, req.url);
        
        // Send a response
        res.writeHead(200, {'Content-Type': 'text/plain'});
        res.end('Hello from Tiny Node.js HTTP Server!');
    });
    
    // Start the server
    print("Starting HTTP server on port 8080...");
    server.listen(8080, function() {
        print("HTTP server is running on http://localhost:8080/");
        
        // In a real application, we would keep the server running,
        // but for this test we'll shut it down shortly
        setTimeout(function() {
            print("Shutting down HTTP server...");
            server.close();
            print("HTTP server shut down");
        }, 1000);
    });
} catch (e) {
    print("HTTP module test failed:", e.message);
}

/**
 * Test process module
 * 
 * This demonstrates using the process module to access information
 * about the current process and environment.
 */
try {
    print("\nTesting process module...");
    
    // Test basic properties
    print("Node.js version:", process.version);
    print("Platform:", process.platform);
    print("Architecture:", process.arch);
    
    // Test current working directory
    print("Current directory:", process.cwd());
    
    // Test command line arguments (limited display)
    print("Command line args:", process.argv.slice(0, 2).join(' '));
    
    // Test environment variables (limited display)
    print("USER env variable:", process.env.USER || process.env.USERNAME || "unknown");
    
    print("Process module test completed successfully");
} catch (e) {
    print("Process module test failed:", e.message);
}

// Keep the runtime alive for a bit to allow async operations to complete
// In a real Node.js application, the process would exit when all tasks are done
setTimeout(function() {
    print("Exiting...");
}, 3000); 