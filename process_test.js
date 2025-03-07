/**
 * Process Module Test Script for Tiny Node.js Runtime
 * 
 * This script demonstrates using the process module in our tiny Node.js implementation.
 * It shows:
 * - Accessing command-line arguments
 * - Getting the current working directory
 * - Accessing environment variables
 * - Exiting the process with a specific code
 * 
 * The process module provides information about and control over the current
 * Node.js process, similar to the global process object in Node.js.
 */

// Import the process module (implemented natively in C++)
const process = require('process');

/**
 * Print command-line arguments
 * 
 * process.argv is an array containing the command-line arguments.
 * The first element is the path to the Node.js executable,
 * the second element is the path to the JavaScript file being executed,
 * and the remaining elements are any additional command-line arguments.
 */
print('Command-line arguments:');
process.argv.forEach((arg, index) => {
    print(`  ${index}: ${arg}`);
});

/**
 * Print current working directory
 * 
 * process.cwd() returns the current working directory of the Node.js process.
 * This is the directory from which the process was started.
 */
print('\nCurrent working directory:');
print(`  ${process.cwd()}`);

/**
 * Print environment variables
 * 
 * process.env is an object containing the user environment variables.
 * These are key-value pairs that can be used to configure the application.
 */
print('\nEnvironment variables:');
Object.keys(process.env).forEach(key => {
    print(`  ${key}=${process.env[key]}`);
});

/**
 * Exit the process after 5 seconds
 * 
 * process.exit(code) terminates the process with the specified exit code.
 * By convention, exit code 0 indicates success, while non-zero codes indicate errors.
 */
print('\nExiting in 5 seconds...');
setTimeout(() => {
    print('Exiting with code 0');
    process.exit(0);
}, 5000); 