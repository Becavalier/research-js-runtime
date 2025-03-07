# 9. HTTP Server Implementation

One of the most powerful features of a JavaScript runtime is the ability to create network servers, particularly HTTP servers. This chapter explores how Tiny Node.js implements its HTTP module, enabling JavaScript code to create and manage web servers.

## The Role of the HTTP Module

The HTTP module allows JavaScript code to:
- Create HTTP servers
- Handle incoming HTTP requests
- Send HTTP responses
- Parse HTTP headers and bodies
- Create HTTP clients (not implemented in our simplified version)

This functionality is essential for building web applications, APIs, and microservices.

## HTTP Module Architecture

The HTTP module in Tiny Node.js consists of several components:

1. **Module Registration**: Registers the HTTP module with the module system
2. **Server Creation**: Provides the ability to create HTTP servers
3. **Request Handling**: Parses and handles incoming HTTP requests
4. **Response Management**: Sends responses back to clients

Let's examine each of these components.

## Module Registration

Like the FS module, the HTTP module is registered with the module system:

```cpp
void RegisterHttpModule(v8::Isolate* isolate, ModuleSystem* module_system) {
    std::cout << "RegisterHttpModule: Starting..." << std::endl;
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    std::cout << "RegisterHttpModule: Got isolate" << std::endl;
    
    // Create and enter a context
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    std::cout << "RegisterHttpModule: Created and entered context" << std::endl;
    
    // Create the http object
    v8::Local<v8::Object> http = v8::Object::New(isolate);
    std::cout << "RegisterHttpModule: Created http object" << std::endl;
    
    // Add the createServer function
    std::cout << "RegisterHttpModule: Adding createServer function..." << std::endl;
    http->Set(
        context,
        v8::String::NewFromUtf8(isolate, "createServer").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, CreateServerCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Register the module with the module system
    std::cout << "RegisterHttpModule: Registering module with ModuleSystem..." << std::endl;
    module_system->RegisterNativeModule("http", http);
    
    std::cout << "RegisterHttpModule: Complete" << std::endl;
}
```

This function:
1. Creates an object that will represent the HTTP module
2. Adds the `createServer` function to this object
3. Registers the object with the module system as the "http" module

## Server Creation

The `createServer` function is the core of the HTTP module, allowing JavaScript code to create an HTTP server:

```cpp
void CreateServerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Expected a request handler function").ToLocalChecked()));
        return;
    }
    
    // Get the request handler function
    v8::Local<v8::Function> request_handler = v8::Local<v8::Function>::Cast(args[0]);
    
    // Create a persistent handle to the request handler
    auto persistent_handler = new v8::Persistent<v8::Function>(isolate, request_handler);
    
    // Get the event loop from the isolate data
    uv_loop_t* event_loop = static_cast<uv_loop_t*>(
        isolate->GetData(EVENT_LOOP_INDEX));
    
    // Create the HTTP server
    struct HttpServer {
        uv_tcp_t server;
        v8::Isolate* isolate;
        v8::Persistent<v8::Function>* request_handler;
    };
    
    auto server_data = new HttpServer;
    server_data->isolate = isolate;
    server_data->request_handler = persistent_handler;
    
    // Initialize the TCP server
    uv_tcp_init(event_loop, &server_data->server);
    server_data->server.data = server_data;
    
    // Create a server object to return to JavaScript
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::ObjectTemplate> server_template = v8::ObjectTemplate::New(isolate);
    
    // Add internal field to store the server handle
    server_template->SetInternalFieldCount(1);
    
    v8::Local<v8::Object> server_obj = server_template->NewInstance(context).ToLocalChecked();
    
    // Add the server handle to the object
    server_obj->SetInternalField(0, v8::External::New(isolate, &server_data->server));
    
    // Add the listen method to the server object
    server_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "listen").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            // Implementation of the listen method
            ListenCallback(args);
        })->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Return the server object
    args.GetReturnValue().Set(server_obj);
}
```

This function:
1. Takes a request handler function as its argument
2. Creates a TCP server using LibUV
3. Returns a JavaScript object with a `listen` method that allows the server to start listening for connections

## The Listen Method

The `listen` method binds the server to a port and starts accepting connections:

```cpp
void ListenCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Get the server object (this)
    v8::Local<v8::Object> server_obj = args.This();
    
    // Get the TCP server handle
    v8::Local<v8::Value> internal = server_obj->GetInternalField(0);
    uv_tcp_t* server = static_cast<uv_tcp_t*>(
        v8::External::Cast(*internal)->Value());
        
    // Get the port
    int port = 8080;  // Default port
    if (args.Length() > 0 && args[0]->IsNumber()) {
        port = args[0]->Int32Value(isolate->GetCurrentContext()).FromJust();
    }
    
    // Bind the server to 0.0.0.0:port
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(server, reinterpret_cast<struct sockaddr*>(&addr), 0);
    
    // Start listening
    uv_listen(reinterpret_cast<uv_stream_t*>(server), 128, [](uv_stream_t* server, int status) {
        // Connection callback
        if (status < 0) {
            std::cerr << "Listen error: " << uv_strerror(status) << std::endl;
            return;
        }
        
        // Accept the connection
        AcceptConnection(server);
    });
    
    // Get the callback function
    v8::Local<v8::Function> callback;
    if (args.Length() > 1 && args[1]->IsFunction()) {
        callback = v8::Local<v8::Function>::Cast(args[1]);
        
        // Call the callback
        v8::Local<v8::Value> argv[] = {};
        callback->Call(isolate->GetCurrentContext(), 
                       isolate->GetCurrentContext()->Global(), 
                       0, argv).ToLocalChecked();
    }
    
    std::cout << "HTTP server starting on port " << port << std::endl;
}
```

This function:
1. Gets the server handle from the server object
2. Gets the port number from the arguments (or uses a default)
3. Binds the server to the specified port
4. Starts listening for connections
5. Sets up a callback to handle incoming connections
6. Calls the JavaScript callback function if provided

## Accepting Connections

When a client connects to the server, the connection is accepted and handled:

```cpp
void AcceptConnection(uv_stream_t* server) {
    // Get the server data
    auto server_data = static_cast<HttpServer*>(server->data);
    
    // Create a client handle
    uv_tcp_t* client = new uv_tcp_t;
    uv_tcp_init(server->loop, client);
    
    if (uv_accept(server, reinterpret_cast<uv_stream_t*>(client)) == 0) {
        // Create a structure to hold request data
        struct HttpRequest {
            uv_tcp_t* client;
            HttpServer* server_data;
            std::string method;
            std::string url;
            std::string version;
            std::string body;
            std::unordered_map<std::string, std::string> headers;
            bool headers_complete;
            size_t content_length;
            size_t body_read;
        };
        
        auto req_data = new HttpRequest;
        req_data->client = client;
        req_data->server_data = server_data;
        req_data->headers_complete = false;
        req_data->content_length = 0;
        req_data->body_read = 0;
        client->data = req_data;
        
        // Start reading from the client
        uv_read_start(reinterpret_cast<uv_stream_t*>(client),
            [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
                // Allocate buffer for reading
                *buf = uv_buf_init(new char[suggested_size], suggested_size);
            },
            [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
                // Handle read completion
                auto req_data = static_cast<HttpRequest*>(stream->data);
                
                if (nread < 0) {
                    // Error or EOF
                    CleanupRequest(stream, buf);
                    return;
                }
                
                if (nread > 0) {
                    // Process the data
                    ProcessHttpData(req_data, buf->base, nread);
                }
                
                delete[] buf->base;
            }
        );
    } else {
        // Failed to accept, clean up
        uv_close(reinterpret_cast<uv_handle_t*>(client), [](uv_handle_t* handle) {
            delete handle;
        });
    }
}
```

This function:
1. Creates a new client handle for the connection
2. Accepts the connection
3. Sets up a structure to hold the HTTP request data
4. Starts reading data from the client
5. Processes the HTTP data as it arrives

## Processing HTTP Data

The HTTP data is processed as it arrives from the client:

```cpp
void ProcessHttpData(HttpRequest* req_data, const char* data, size_t length) {
    // If headers are not yet complete, parse them
    if (!req_data->headers_complete) {
        std::string chunk(data, length);
        size_t header_end = chunk.find("\r\n\r\n");
        
        if (header_end != std::string::npos) {
            // Headers are complete
            std::string headers = chunk.substr(0, header_end);
            
            // Parse the headers
            std::istringstream header_stream(headers);
            std::string line;
            
            // Parse the request line
            std::getline(header_stream, line);
            size_t method_end = line.find(' ');
            size_t url_end = line.find(' ', method_end + 1);
            
            req_data->method = line.substr(0, method_end);
            req_data->url = line.substr(method_end + 1, url_end - method_end - 1);
            req_data->version = line.substr(url_end + 1);
            
            // Parse the header fields
            while (std::getline(header_stream, line) && line != "\r") {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    
                    // Trim whitespace
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of("\r") + 1);
                    
                    req_data->headers[key] = value;
                }
            }
            
            // Check for Content-Length
            auto it = req_data->headers.find("Content-Length");
            if (it != req_data->headers.end()) {
                req_data->content_length = std::stoi(it->second);
            }
            
            // Mark headers as complete
            req_data->headers_complete = true;
            
            // Process any body data that came with the headers
            if (header_end + 4 < length) {
                size_t body_length = length - (header_end + 4);
                req_data->body.append(data + header_end + 4, body_length);
                req_data->body_read += body_length;
            }
        } else {
            // Headers are not yet complete, store this chunk
            // (In a real implementation, you'd need to handle larger headers)
            std::cerr << "Headers not complete in this chunk" << std::endl;
            return;
        }
    } else {
        // Headers are already complete, this is body data
        req_data->body.append(data, length);
        req_data->body_read += length;
    }
    
    // Check if we've received the complete body
    if (req_data->headers_complete && req_data->body_read >= req_data->content_length) {
        // Request is complete, call the request handler
        HandleCompleteRequest(req_data);
    }
}
```

This function:
1. Parses the HTTP headers if they're not yet complete
2. Extracts the method, URL, and HTTP version from the request line
3. Parses the header fields
4. Processes any body data that came with the headers
5. Updates the body data as more chunks arrive
6. Checks if the complete request has been received
7. Calls the request handler when the request is complete

## Handling Complete Requests

When a complete HTTP request has been received, it's passed to the JavaScript request handler:

```cpp
void HandleCompleteRequest(HttpRequest* req_data) {
    // Get the isolate and request handler
    v8::Isolate* isolate = req_data->server_data->isolate;
    v8::Persistent<v8::Function>* request_handler = req_data->server_data->request_handler;
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    
    // Get the current context
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);
    
    // Create the request object
    v8::Local<v8::Object> req_obj = v8::Object::New(isolate);
    
    // Set the method
    req_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "method").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, req_data->method.c_str()).ToLocalChecked()
    ).Check();
    
    // Set the URL
    req_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "url").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, req_data->url.c_str()).ToLocalChecked()
    ).Check();
    
    // Set the headers
    v8::Local<v8::Object> headers_obj = v8::Object::New(isolate);
    for (const auto& header : req_data->headers) {
        headers_obj->Set(
            context,
            v8::String::NewFromUtf8(isolate, header.first.c_str()).ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, header.second.c_str()).ToLocalChecked()
        ).Check();
    }
    
    req_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "headers").ToLocalChecked(),
        headers_obj
    ).Check();
    
    // Set the body
    req_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "body").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, req_data->body.c_str()).ToLocalChecked()
    ).Check();
    
    // Create the response object
    v8::Local<v8::Object> res_obj = CreateResponseObject(isolate, context, req_data);
    
    // Call the request handler
    v8::Local<v8::Function> handler = v8::Local<v8::Function>::New(isolate, *request_handler);
    v8::Local<v8::Value> argv[] = { req_obj, res_obj };
    handler->Call(context, context->Global(), 2, argv).ToLocalChecked();
}
```

This function:
1. Creates a JavaScript request object with the parsed HTTP request data
2. Creates a JavaScript response object that can be used to send the response
3. Calls the JavaScript request handler with these objects

## Creating the Response Object

The response object provides methods for sending an HTTP response:

```cpp
v8::Local<v8::Object> CreateResponseObject(v8::Isolate* isolate, v8::Local<v8::Context> context, HttpRequest* req_data) {
    // Create the response object
    v8::Local<v8::Object> res_obj = v8::Object::New(isolate);
    
    // Add internal field to store the request data
    res_obj->SetInternalField(0, v8::External::New(isolate, req_data));
    
    // Set default status code
    res_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "statusCode").ToLocalChecked(),
        v8::Number::New(isolate, 200)
    ).Check();
    
    // Add the setHeader method
    res_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "setHeader").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            // Implementation of the setHeader method
            SetHeaderCallback(args);
        })->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Add the end method
    res_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "end").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            // Implementation of the end method
            EndCallback(args);
        })->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Create headers object
    v8::Local<v8::Object> headers_obj = v8::Object::New(isolate);
    
    // Set default headers
    headers_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "Content-Type").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, "text/plain").ToLocalChecked()
    ).Check();
    
    // Add headers object to response
    res_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "headers").ToLocalChecked(),
        headers_obj
    ).Check();
    
    return res_obj;
}
```

This function:
1. Creates a JavaScript object to represent the HTTP response
2. Sets default values like status code and content type
3. Adds methods for setting headers and ending the response
4. Returns the response object

## Setting Response Headers

The `setHeader` method allows setting HTTP response headers:

```cpp
void SetHeaderCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    // Check arguments
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Expected header name and value").ToLocalChecked()));
        return;
    }
    
    // Get the response object
    v8::Local<v8::Object> res_obj = args.This();
    
    // Get the headers object
    v8::Local<v8::Value> headers_value;
    if (!res_obj->Get(context, v8::String::NewFromUtf8(isolate, "headers").ToLocalChecked()).ToLocal(&headers_value) || !headers_value->IsObject()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Headers object not found").ToLocalChecked()));
        return;
    }
    
    v8::Local<v8::Object> headers_obj = headers_value.As<v8::Object>();
    
    // Get the header name and value
    v8::String::Utf8Value name(isolate, args[0]);
    v8::String::Utf8Value value(isolate, args[1]);
    
    // Set the header
    headers_obj->Set(
        context,
        args[0],
        args[1]
    ).Check();
    
    // Return undefined
    args.GetReturnValue().SetUndefined();
}
```

This function:
1. Gets the header name and value from the arguments
2. Gets the headers object from the response object
3. Sets the header in the headers object

## Ending the Response

The `end` method sends the HTTP response and closes the connection:

```cpp
void EndCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    // Get the response object
    v8::Local<v8::Object> res_obj = args.This();
    
    // Get the request data
    v8::Local<v8::Value> internal = res_obj->GetInternalField(0);
    HttpRequest* req_data = static_cast<HttpRequest*>(
        v8::External::Cast(*internal)->Value());
    
    // Get the status code
    v8::Local<v8::Value> status_value;
    if (!res_obj->Get(context, v8::String::NewFromUtf8(isolate, "statusCode").ToLocalChecked()).ToLocal(&status_value) || !status_value->IsNumber()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Status code not found").ToLocalChecked()));
        return;
    }
    
    int status_code = status_value->Int32Value(context).FromJust();
    
    // Get the headers
    v8::Local<v8::Value> headers_value;
    if (!res_obj->Get(context, v8::String::NewFromUtf8(isolate, "headers").ToLocalChecked()).ToLocal(&headers_value) || !headers_value->IsObject()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Headers object not found").ToLocalChecked()));
        return;
    }
    
    v8::Local<v8::Object> headers_obj = headers_value.As<v8::Object>();
    
    // Get the response body
    std::string body;
    if (args.Length() > 0) {
        if (args[0]->IsString()) {
            v8::String::Utf8Value body_str(isolate, args[0]);
            body = *body_str;
        } else {
            // Convert to string
            v8::Local<v8::String> body_str;
            if (!args[0]->ToString(context).ToLocal(&body_str)) {
                isolate->ThrowException(v8::Exception::Error(
                    v8::String::NewFromUtf8(isolate, "Could not convert body to string").ToLocalChecked()));
                return;
            }
            
            v8::String::Utf8Value body_utf8(isolate, body_str);
            body = *body_utf8;
        }
    }
    
    // Build the response
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 " << status_code << " ";
    if (status_code == 200) {
        response << "OK";
    } else if (status_code == 404) {
        response << "Not Found";
    } else if (status_code == 500) {
        response << "Internal Server Error";
    } else {
        response << "Unknown";
    }
    response << "\r\n";
    
    // Headers
    v8::Local<v8::Array> header_names = headers_obj->GetOwnPropertyNames(context).ToLocalChecked();
    for (uint32_t i = 0; i < header_names->Length(); i++) {
        v8::Local<v8::Value> name = header_names->Get(context, i).ToLocalChecked();
        v8::Local<v8::Value> value = headers_obj->Get(context, name).ToLocalChecked();
        
        v8::String::Utf8Value name_str(isolate, name);
        v8::String::Utf8Value value_str(isolate, value);
        
        response << *name_str << ": " << *value_str << "\r\n";
    }
    
    // Content-Length
    response << "Content-Length: " << body.length() << "\r\n";
    
    // End of headers
    response << "\r\n";
    
    // Body
    response << body;
    
    // Send the response
    std::string response_str = response.str();
    uv_buf_t buf = uv_buf_init(const_cast<char*>(response_str.c_str()), response_str.length());
    
    uv_write_t* write_req = new uv_write_t;
    write_req->data = req_data;
    
    uv_write(write_req, reinterpret_cast<uv_stream_t*>(req_data->client), &buf, 1, [](uv_write_t* req, int status) {
        // Write complete, clean up
        HttpRequest* req_data = static_cast<HttpRequest*>(req->data);
        
        // Close the connection
        uv_close(reinterpret_cast<uv_handle_t*>(req_data->client), [](uv_handle_t* handle) {
            HttpRequest* req_data = static_cast<HttpRequest*>(handle->data);
            delete req_data;
            delete handle;
        });
        
        delete req;
    });
    
    // Log the response
    std::cout << "Response status: " << status_code << std::endl;
    std::cout << "Response body: " << body << std::endl;
    
    // Return undefined
    args.GetReturnValue().SetUndefined();
}
```

This function:
1. Gets the status code, headers, and body for the response
2. Builds the HTTP response string
3. Sends the response to the client
4. Closes the connection
5. Cleans up resources

## Using the HTTP Module in JavaScript

Once the HTTP module is implemented and registered, it can be used in JavaScript code like this:

```javascript
// Import the http module
const http = require('http');

// Create an HTTP server
const server = http.createServer((req, res) => {
    // Log the request
    console.log(`Received ${req.method} request for ${req.url}`);
    
    // Set response headers
    res.setHeader('Content-Type', 'text/html');
    
    // Set status code if needed
    res.statusCode = 200;
    
    // Send the response
    res.end(`
        <html>
            <head>
                <title>Tiny Node.js Server</title>
            </head>
            <body>
                <h1>Hello from Tiny Node.js HTTP Server!</h1>
                <p>URL: ${req.url}</p>
                <p>Method: ${req.method}</p>
            </body>
        </html>
    `);
});

// Start the server on port 3000
server.listen(3000, () => {
    console.log('Server running at http://localhost:3000/');
});
```

This example:
1. Creates an HTTP server with a request handler function
2. Sets response headers and status code
3. Sends an HTML response
4. Starts the server on port 3000

## Advanced HTTP Features

Our implementation is simplified, but a more complete HTTP module would include:

1. **HTTP Clients**: For making HTTP requests to other servers
2. **Streaming**: For efficiently handling large requests and responses
3. **Compression**: For compressing response data
4. **Cookies**: For managing HTTP cookies
5. **More thorough parsing**: For handling complex HTTP requests
6. **HTTPS support**: For secure HTTP connections

## Error Handling

Proper error handling is crucial for HTTP servers. In our implementation, errors are handled in several places:

1. **Connection errors**: When accepting connections
2. **Read errors**: When reading request data
3. **Write errors**: When sending responses
4. **Parse errors**: When parsing HTTP headers and bodies

These errors are logged and the connection is closed to prevent resource leaks.

## Performance Considerations

HTTP servers can be performance-critical, especially in production environments. Here are some considerations:

1. **Connection pooling**: Reuse connections for multiple requests
2. **Request parsing**: Efficiently parse HTTP headers and bodies
3. **Response buffering**: Buffer responses for efficient sending
4. **Memory management**: Properly clean up resources
5. **Error handling**: Recover gracefully from errors

## Security Considerations

HTTP servers can be vulnerable to various attacks. Here are some security considerations:

1. **Input validation**: Validate all user input
2. **Header validation**: Check for malicious headers
3. **Request size limits**: Limit the size of requests
4. **Rate limiting**: Limit the number of requests per client
5. **HTTPS**: Use HTTPS for secure connections

## Conclusion

The HTTP module is a powerful component of Tiny Node.js, enabling JavaScript code to create and manage web servers. By implementing a basic HTTP server, we've provided a foundation for building web applications and APIs.

Understanding the implementation of the HTTP module provides insights into:
- How network servers are implemented in a JavaScript runtime
- How the event loop is used for asynchronous I/O
- How C++ and JavaScript code work together to provide high-level functionality

[← Previous: File System Operations](08-fs-module.md) | [Next: Process Module →](10-process-module.md) 