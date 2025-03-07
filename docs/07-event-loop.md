# 7. Event Loop

The event loop is a fundamental concept in JavaScript runtimes, enabling asynchronous, non-blocking operations. This chapter explores how Tiny Node.js implements its event loop using the LibUV library, and how this enables features like timers, I/O operations, and HTTP servers.

## What is an Event Loop?

At its core, an event loop is a programming construct that waits for and dispatches events or messages in a program. In the context of a JavaScript runtime, the event loop enables:

- **Non-blocking operations**: Operations like file I/O don't freeze the program while waiting for completion
- **Asynchronous callbacks**: Functions can be scheduled to run at a later time
- **Event-driven programming**: Code can respond to events like timers, I/O completion, and HTTP requests

The event loop is what makes JavaScript's asynchronous nature possible, allowing operations to continue while waiting for long-running tasks to complete.

## How Event Loops Work

The basic structure of an event loop is as follows:

1. **Initialize**: Set up data structures and prepare the event loop
2. **Poll for events**: Check for new events or if any previous events are ready
3. **Process events**: Execute callbacks for ready events
4. **Sleep**: If no events are ready, sleep until new events arrive
5. **Repeat**: Go back to step 2 until the event loop is stopped

This pattern allows the program to efficiently handle many concurrent operations without creating a thread for each one.

## LibUV: The Event Loop Library

Tiny Node.js uses [LibUV](https://libuv.org/) as its event loop implementation. LibUV is the same library used by Node.js, providing:

- Cross-platform support for asynchronous I/O
- Event loop implementation
- Timer functionality
- File system operations
- Networking operations (TCP, UDP)
- Process and thread management

## Integrating LibUV in Tiny Node.js

### Initialization

The event loop is initialized in the `Runtime` constructor:

```cpp
Runtime::Runtime(int argc, char* argv[]) {
    // ... (earlier code)
    
    // Create event loop for asynchronous operations
    std::cout << "Runtime constructor: Creating event loop..." << std::endl;
    event_loop_ = new uv_loop_t;
    uv_loop_init(event_loop_);
    
    // ... (later code)
}
```

This creates a new LibUV event loop and initializes it. The event loop is stored as a member variable of the `Runtime` class.

### Running the Event Loop

The event loop is run after executing the JavaScript file:

```cpp
bool Runtime::ExecuteFile(const std::string& filename) {
    // ... (earlier code)
    
    // Execute the file content
    bool result = ExecuteString(source, filename);
    
    if (result) {
        std::cout << "File executed successfully, waiting for event loop..." << std::endl;
        
        // Run the event loop until it's empty
        uv_run(event_loop_, UV_RUN_DEFAULT);
    }
    
    // ... (later code)
    
    return result;
}
```

The `uv_run` function starts the event loop, which processes events until there are no more active handles or requests. The `UV_RUN_DEFAULT` mode means the loop will run until there are no more active and referenced handles or requests.

### Cleanup

The event loop is cleaned up in the `Runtime` destructor and `Shutdown` method:

```cpp
Runtime::~Runtime() {
    if (event_loop_) {
        uv_loop_close(event_loop_);
        delete event_loop_;
    }
    
    // ... (other cleanup)
}

void Runtime::Shutdown() {
    std::cout << "Shutdown: Starting..." << std::endl;
    
    // Clean up the event loop
    uv_loop_close(event_loop_);
    delete event_loop_;
    event_loop_ = nullptr;
    
    std::cout << "Shutdown: Complete" << std::endl;
}
```

These methods ensure that the event loop is properly closed and its memory is freed.

## Implementing Asynchronous Operations

Let's look at how different types of asynchronous operations are implemented using the event loop.

### Timers

Timers allow scheduling a function to run after a delay. The `setTimeout` function is implemented as follows:

```cpp
void SetTimeoutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 2 || !args[0]->IsFunction() || !args[1]->IsNumber()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the callback function
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[0]);
    
    // Get the delay in milliseconds
    int delay_ms = args[1]->Int32Value(isolate->GetCurrentContext()).FromJust();
    
    // Create a persistent handle to the callback
    auto persistent_callback = new v8::Persistent<v8::Function>(isolate, callback);
    
    // Get the event loop from the isolate data
    uv_loop_t* event_loop = static_cast<uv_loop_t*>(
        isolate->GetData(EVENT_LOOP_INDEX));
    
    // Create a timer
    uv_timer_t* timer = new uv_timer_t;
    timer->data = persistent_callback;
    
    // Initialize the timer
    uv_timer_init(event_loop, timer);
    
    // Start the timer
    uv_timer_start(timer, [](uv_timer_t* handle) {
        // Get the persistent callback
        auto persistent_callback = static_cast<v8::Persistent<v8::Function>*>(handle->data);
        
        // Get the isolate
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        
        // Create a handle scope
        v8::HandleScope handle_scope(isolate);
        
        // Get the current context
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        
        // Create a local handle to the callback
        v8::Local<v8::Function> callback = 
            v8::Local<v8::Function>::New(isolate, *persistent_callback);
        
        // Call the callback
        callback->Call(context, context->Global(), 0, nullptr).ToLocalChecked();
        
        // Clean up
        persistent_callback->Reset();
        delete persistent_callback;
        uv_timer_stop(handle);
        uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle) {
            delete reinterpret_cast<uv_timer_t*>(handle);
        });
        
        std::cout << "Timeout executed after " << handle->timeout << "ms" << std::endl;
    }, delay_ms, 0);
    
    // Return the timer handle as the timeout ID
    args.GetReturnValue().Set(v8::External::New(isolate, timer));
}
```

This function:
1. Creates a LibUV timer
2. Sets up a callback to be called when the timer expires
3. Starts the timer with the specified delay
4. Returns a handle to the timer for potential cancellation

When the timer expires, the callback is executed within the event loop, ensuring it runs on the main JavaScript thread.

### File System Operations

File system operations, like reading a file, can be made asynchronous using the event loop. Here's a simplified implementation of an asynchronous `readFile` function:

```cpp
void ReadFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the filename
    v8::String::Utf8Value filename(isolate, args[0]);
    
    // Get the callback function
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[1]);
    
    // Create a persistent handle to the callback
    auto persistent_callback = new v8::Persistent<v8::Function>(isolate, callback);
    
    // Get the event loop from the isolate data
    uv_loop_t* event_loop = static_cast<uv_loop_t*>(
        isolate->GetData(EVENT_LOOP_INDEX));
    
    // Create a structure to hold file data
    struct FileReadData {
        uv_fs_t req;
        uv_buf_t buf;
        v8::Isolate* isolate;
        v8::Persistent<v8::Function>* callback;
    };
    
    auto data = new FileReadData;
    data->isolate = isolate;
    data->callback = persistent_callback;
    
    // Open the file asynchronously
    data->req.data = data;
    uv_fs_open(event_loop, &data->req, *filename, O_RDONLY, 0, [](uv_fs_t* req) {
        auto data = static_cast<FileReadData*>(req->data);
        
        if (req->result < 0) {
            // Handle error
            InvokeCallback(data->isolate, data->callback, 
                           uv_strerror(req->result), nullptr);
            delete data->callback;
            delete data;
            uv_fs_req_cleanup(req);
            return;
        }
        
        // File opened successfully, now read it
        int fd = req->result;
        uv_fs_req_cleanup(req);
        
        // Allocate buffer
        data->buf = uv_buf_init(new char[1024], 1024);
        
        // Read the file
        uv_fs_read(req->loop, req, fd, &data->buf, 1, 0, [](uv_fs_t* req) {
            auto data = static_cast<FileReadData*>(req->data);
            
            if (req->result < 0) {
                // Handle error
                InvokeCallback(data->isolate, data->callback, 
                               uv_strerror(req->result), nullptr);
            } else if (req->result == 0) {
                // End of file, close and cleanup
                InvokeCallback(data->isolate, data->callback, nullptr, "");
            } else {
                // Data read, process it
                std::string content(data->buf.base, req->result);
                InvokeCallback(data->isolate, data->callback, nullptr, content.c_str());
            }
            
            // Close the file
            int fd = req->file;
            uv_fs_req_cleanup(req);
            uv_fs_close(req->loop, req, fd, [](uv_fs_t* req) {
                auto data = static_cast<FileReadData*>(req->data);
                
                // Clean up
                delete[] data->buf.base;
                delete data->callback;
                delete data;
                uv_fs_req_cleanup(req);
            });
        });
    });
}

// Helper function to invoke the JavaScript callback
void InvokeCallback(v8::Isolate* isolate, v8::Persistent<v8::Function>* callback,
                    const char* error, const char* data) {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Function> cb = v8::Local<v8::Function>::New(isolate, *callback);
    
    v8::Local<v8::Value> argv[2];
    if (error) {
        argv[0] = v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, error).ToLocalChecked());
        argv[1] = v8::Null(isolate);
    } else {
        argv[0] = v8::Null(isolate);
        argv[1] = v8::String::NewFromUtf8(isolate, data).ToLocalChecked();
    }
    
    cb->Call(context, context->Global(), 2, argv).ToLocalChecked();
}
```

This function:
1. Opens the file asynchronously using LibUV's `uv_fs_open`
2. When the file is opened, reads it asynchronously using `uv_fs_read`
3. When the read is complete, closes the file and invokes the JavaScript callback
4. All operations are performed within the event loop, so they don't block the JavaScript execution

### HTTP Server

The HTTP server implementation also uses the event loop for handling connections and requests:

```cpp
void CreateServerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
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
    v8::Local<v8::Object> server_obj = v8::Object::New(isolate);
    
    // Add the server handle to the object
    server_obj->SetInternalField(0, v8::External::New(isolate, &server_data->server));
    
    // Add the listen method to the server object
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    server_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "listen").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
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
            
            // Bind the server to localhost:port
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", port, &addr);
            uv_tcp_bind(server, reinterpret_cast<struct sockaddr*>(&addr), 0);
            
            // Start listening
            uv_listen(reinterpret_cast<uv_stream_t*>(server), 128, [](uv_stream_t* server, int status) {
                if (status < 0) {
                    std::cerr << "Listen error: " << uv_strerror(status) << std::endl;
                    return;
                }
                
                // Get the server data
                auto server_data = static_cast<HttpServer*>(server->data);
                
                // Accept the connection
                uv_tcp_t* client = new uv_tcp_t;
                uv_tcp_init(server->loop, client);
                
                if (uv_accept(server, reinterpret_cast<uv_stream_t*>(client)) == 0) {
                    // Create a structure to hold request data
                    struct HttpRequest {
                        uv_tcp_t* client;
                        HttpServer* server_data;
                        std::string method;
                        std::string url;
                        std::string body;
                        std::unordered_map<std::string, std::string> headers;
                    };
                    
                    auto req_data = new HttpRequest;
                    req_data->client = client;
                    req_data->server_data = server_data;
                    client->data = req_data;
                    
                    // Allocate buffer for reading
                    uv_buf_t buf = uv_buf_init(new char[4096], 4096);
                    
                    // Read the request
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
                                if (nread != UV_EOF) {
                                    std::cerr << "Read error: " << uv_strerror(nread) << std::endl;
                                }
                                
                                // Clean up
                                delete[] buf->base;
                                uv_close(reinterpret_cast<uv_handle_t*>(req_data->client), [](uv_handle_t* handle) {
                                    auto req_data = static_cast<HttpRequest*>(handle->data);
                                    delete req_data;
                                    delete handle;
                                });
                                return;
                            }
                            
                            if (nread > 0) {
                                // Parse the HTTP request
                                std::string data(buf->base, nread);
                                // ... (HTTP parsing code omitted for brevity)
                                
                                // Create request and response objects for JavaScript
                                v8::Isolate* isolate = req_data->server_data->isolate;
                                v8::HandleScope handle_scope(isolate);
                                v8::Local<v8::Context> context = isolate->GetCurrentContext();
                                
                                v8::Local<v8::Object> req_obj = v8::Object::New(isolate);
                                v8::Local<v8::Object> res_obj = v8::Object::New(isolate);
                                
                                // ... (Set up request and response objects)
                                
                                // Call the request handler
                                v8::Local<v8::Function> handler = v8::Local<v8::Function>::New(
                                    isolate, *req_data->server_data->request_handler);
                                    
                                v8::Local<v8::Value> argv[] = { req_obj, res_obj };
                                handler->Call(context, context->Global(), 2, argv).ToLocalChecked();
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
        })->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Return the server object
    args.GetReturnValue().Set(server_obj);
}
```

This function:
1. Creates a TCP server using LibUV
2. Sets up a listen method that binds to a port and starts accepting connections
3. When a connection is accepted, reads the request data asynchronously
4. Parses the HTTP request and creates request and response objects
5. Calls the JavaScript request handler with these objects
6. All of this happens within the event loop, allowing the server to handle multiple connections concurrently

## Event Loop Phases

The LibUV event loop has several phases that it goes through in each iteration:

1. **Timer Phase**: Executes callbacks scheduled by `setTimeout` and `setInterval`
2. **Pending Callbacks Phase**: Executes I/O callbacks deferred from the previous iteration
3. **Idle, Prepare Phase**: Used internally
4. **Poll Phase**: Waits for new I/O events and executes I/O callbacks
5. **Check Phase**: Executes callbacks scheduled by `setImmediate`
6. **Close Callbacks Phase**: Executes close callbacks for handles being closed

Understanding these phases is important for understanding the order in which callbacks are executed.

## Advanced Event Loop Concepts

### Non-blocking vs. Blocking Operations

One of the key advantages of the event loop is its ability to handle non-blocking operations. For example:

```javascript
// Non-blocking (asynchronous)
fs.readFile('file.txt', (err, data) => {
    console.log(data);
});
console.log('This will run before the file is read');

// Blocking (synchronous)
const data = fs.readFileSync('file.txt');
console.log(data);
console.log('This will run after the file is read');
```

In the non-blocking version, the `readFile` function schedules the file read operation and returns immediately, allowing the code to continue executing. When the file read is complete, the callback is added to the event loop's queue and executed when it reaches the front of the queue.

### Microtasks and Macrotasks

JavaScript has two types of tasks:
- **Macrotasks**: Regular tasks like `setTimeout`, I/O operations, and UI rendering
- **Microtasks**: Tasks that should be executed immediately after the current task, like `Promise` callbacks and `process.nextTick`

The event loop processes all microtasks after each macrotask, before moving on to the next macrotask. This is important for understanding the execution order of callbacks.

### Thread Pool

LibUV uses a thread pool for operations that would otherwise block the event loop, like file I/O and DNS lookups. This allows these operations to be performed asynchronously, without blocking the main JavaScript thread.

## Extending the Event Loop

### Adding Custom Handles

You can extend the event loop by adding custom handles:

```cpp
// Create a custom handle
struct MyHandle {
    uv_handle_t handle;
    v8::Isolate* isolate;
    v8::Persistent<v8::Function>* callback;
    // ... (other data)
};

// Initialize the handle
MyHandle* handle = new MyHandle;
uv_handle_init(event_loop, &handle->handle);
handle->isolate = isolate;
handle->callback = new v8::Persistent<v8::Function>(isolate, callback);
handle->handle.data = handle;

// Add it to the event loop
// ... (custom logic)

// Clean up when done
uv_close(&handle->handle, [](uv_handle_t* h) {
    MyHandle* handle = static_cast<MyHandle*>(h->data);
    handle->callback->Reset();
    delete handle->callback;
    delete handle;
});
```

### Adding Custom Events

You can also add custom events to the event loop:

```cpp
// Create an async handle
uv_async_t* async = new uv_async_t;
async->data = /* your data */;

// Initialize the async handle
uv_async_init(event_loop, async, [](uv_async_t* handle) {
    // This callback will be called when uv_async_send is called
    // ... (custom logic)
});

// Trigger the async handle from another thread
std::thread([async]() {
    // Do work in this thread
    
    // Signal the event loop
    uv_async_send(async);
}).detach();
```

This allows you to integrate custom threading with the event loop, while ensuring that callbacks run on the main JavaScript thread.

## Conclusion

The event loop is the heart of any JavaScript runtime, enabling asynchronous, non-blocking operations. By leveraging LibUV, Tiny Node.js provides a solid foundation for implementing timers, file I/O, networking, and other asynchronous features.

Understanding the event loop is crucial for building and extending a JavaScript runtime, as it determines how and when callbacks are executed, and enables the efficient handling of concurrent operations.

[← Previous: Module System](06-module-system.md) | [Next: File System Operations →](08-fs-module.md) 