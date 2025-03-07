/**
 * hello.js - A simple example of using the Tiny Node.js runtime
 * 
 * This example demonstrates:
 * - Basic JavaScript execution
 * - Using the print function
 * - Using setTimeout for delayed execution
 * - Accessing process information
 */

// Print a greeting
print("Hello from Tiny Node.js!");

// Print some process information
print("\nRuntime Information:");
print("-------------------");
print(`Platform: ${process.platform}`);
print(`Architecture: ${process.arch}`);
print(`Current directory: ${process.cwd()}`);
print(`Node.js version: ${process.version}`);

// Get command line arguments
print("\nCommand Line Arguments:");
print("---------------------");
process.argv.forEach((arg, index) => {
    print(`  argv[${index}] = ${arg}`);
});

// Demonstrate setTimeout
print("\nTesting setTimeout...");
setTimeout(() => {
    print("This message appears after 1 second!");
    print("\nExample complete!");
}, 1000);

// The script will wait for the timeout before exiting
print("Waiting for timeout..."); 