/**
 * HTTP Server Test Script for Tiny Node.js Runtime
 * 
 * This script demonstrates creating and running a simple HTTP server
 * using our tiny Node.js implementation. It shows:
 * - Loading a native module (http)
 * - Creating an HTTP server with a request handler
 * - Starting the server on a specific port
 * - Handling HTTP requests and sending responses
 * 
 * To test this server:
 * 1. Run this script with the tiny Node.js runtime
 * 2. Open a web browser and navigate to http://localhost:3000/
 * 3. You should see "Hello from Tiny Node.js HTTP Server!" in the browser
 */

// Import the http module (implemented natively in C++)
const http = require('http');

/**
 * Create an HTTP server with a request handler
 * 
 * The callback function is called for each incoming HTTP request.
 * It receives two objects:
 * - req: Contains information about the request (method, URL, headers, etc.)
 * - res: Provides methods for sending the response
 */
const server = http.createServer((req, res) => {
    // Log information about the incoming request
    print(`Received ${req.method} request for ${req.url}`);
    
    // Send an HTTP response
    // First, set the status code and headers
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    
    // Then, send the response body and end the response
    res.end('Hello from Tiny Node.js HTTP Server!');
});

// Start the server on port 3000
const PORT = 3000;
server.listen(PORT);
print(`Server running at http://localhost:${PORT}/`);

// Keep the server running for 1 minute
// In a real application, the server would run until explicitly stopped
setTimeout(() => {
    print('Server shutting down...');
}, 60000); // 60000 ms = 1 minute 