#ifndef TINY_NODEJS_HTTP_MODULE_H
#define TINY_NODEJS_HTTP_MODULE_H

// Forward declaration
class Runtime;

/**
 * @brief Register the HTTP module with the runtime
 * 
 * This function creates and registers the HTTP module, which provides
 * functionality for creating HTTP servers, similar to Node.js's http module.
 * 
 * The HTTP module exposes the following functionality to JavaScript:
 * - http.createServer(callback): Creates an HTTP server
 * - server.listen(port): Starts the server on the specified port
 * 
 * The callback function passed to createServer receives request and response objects:
 * - request: Contains information about the HTTP request (method, url, etc.)
 * - response: Provides methods for sending the HTTP response (writeHead, end, etc.)
 * 
 * @param runtime Pointer to the Runtime instance
 */
void RegisterHttpModule(Runtime* runtime);

#endif // TINY_NODEJS_HTTP_MODULE_H 