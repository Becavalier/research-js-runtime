/**
 * server.js - Simple HTTP server example for Tiny Node.js runtime
 * 
 * This example demonstrates:
 * - Loading and using the HTTP module
 * - Creating a basic web server
 * - Handling HTTP requests
 * - Using the event loop
 */

// Load the HTTP module
const http = require('http');

// Configure the server port
const PORT = 3000;

// Create an HTTP server
const server = http.createServer((req, res) => {
    // Print request information
    print(`Received ${req.method} request for ${req.url}`);
    
    // Add a timestamp to the response
    const now = new Date().toISOString();
    
    // Set response headers
    res.statusCode = 200;
    
    // Different responses based on the request path
    if (req.url === '/') {
        // Home page
        res.setHeader('Content-Type', 'text/html');
        res.end(`
            <html>
                <head>
                    <title>Tiny Node.js Server</title>
                    <style>
                        body { font-family: Arial, sans-serif; margin: 40px; line-height: 1.6; }
                        h1 { color: #0066cc; }
                        .info { background-color: #f0f0f0; padding: 15px; border-radius: 5px; }
                    </style>
                </head>
                <body>
                    <h1>Hello from Tiny Node.js HTTP Server!</h1>
                    <div class="info">
                        <p>Server Time: ${now}</p>
                        <p>Try these paths:</p>
                        <ul>
                            <li><a href="/info">Server Info</a></li>
                            <li><a href="/time">Current Time</a></li>
                        </ul>
                    </div>
                </body>
            </html>
        `);
    } else if (req.url === '/info') {
        // Server info page
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify({
            runtime: 'Tiny Node.js',
            version: process.version,
            platform: process.platform,
            architecture: process.arch,
            timestamp: now
        }, null, 2));
    } else if (req.url === '/time') {
        // Time page
        res.setHeader('Content-Type', 'text/plain');
        res.end(`Current server time: ${now}`);
    } else {
        // Not found
        res.statusCode = 404;
        res.setHeader('Content-Type', 'text/plain');
        res.end('404 - Page Not Found');
    }
});

// Start the server
server.listen(PORT, () => {
    print(`\nServer running at http://localhost:${PORT}/`);
    print("Try opening this URL in your web browser\n");
    print("Available endpoints:");
    print("  - http://localhost:3000/       (HTML home page)");
    print("  - http://localhost:3000/info   (JSON server info)");
    print("  - http://localhost:3000/time   (Plain text current time)");
    print("\nPress Ctrl+C to stop the server");
}); 