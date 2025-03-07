# 8. File System Operations

File system operations are essential for any practical JavaScript runtime, allowing scripts to read and write files, check if files exist, and manipulate the file system. This chapter explores how Tiny Node.js implements its file system (fs) module.

## The Role of the FS Module

The file system module provides JavaScript code with access to the operating system's file system. It enables operations such as:

- Reading files
- Writing files
- Checking if files exist
- Getting file information
- Creating, renaming, and deleting files and directories

These capabilities are crucial for a wide range of applications, from simple scripts to complex server-side applications.

## Implementing the FS Module in Tiny Node.js

The fs module in Tiny Node.js is implemented as a native module, which means it's implemented in C++ and exposed to JavaScript through the module system. Let's explore how this is achieved.

### Module Registration

The fs module is registered with the module system in the `RegisterFsModule` function:

```cpp
void RegisterFsModule(v8::Isolate* isolate, ModuleSystem* module_system) {
    std::cout << "RegisterFsModule: Starting..." << std::endl;
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    std::cout << "RegisterFsModule: Got isolate" << std::endl;
    
    // Create and enter a context
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    std::cout << "RegisterFsModule: Created and entered context" << std::endl;
    
    // Create the fs object
    v8::Local<v8::Object> fs = v8::Object::New(isolate);
    std::cout << "RegisterFsModule: Created fs object" << std::endl;
    
    // Add the readFile function
    std::cout << "RegisterFsModule: Adding readFile function..." << std::endl;
    fs->Set(
        context,
        v8::String::NewFromUtf8(isolate, "readFile").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, ReadFileCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Add the writeFile function
    std::cout << "RegisterFsModule: Adding writeFile function..." << std::endl;
    fs->Set(
        context,
        v8::String::NewFromUtf8(isolate, "writeFile").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, WriteFileCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Add the exists function
    std::cout << "RegisterFsModule: Adding exists function..." << std::endl;
    fs->Set(
        context,
        v8::String::NewFromUtf8(isolate, "exists").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, ExistsCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Register the module with the module system
    std::cout << "RegisterFsModule: Registering module with ModuleSystem..." << std::endl;
    module_system->RegisterNativeModule("fs", fs);
    
    std::cout << "RegisterFsModule: Complete" << std::endl;
}
```

This function:
1. Creates an object that will represent the fs module
2. Adds functions like `readFile`, `writeFile`, and `exists` to this object
3. Registers the object with the module system as the "fs" module
4. Provides logging to track the registration process

### Reading Files

The `readFile` function allows JavaScript code to read the contents of a file. Let's examine the synchronous version first:

```cpp
void ReadFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Expected a filename string").ToLocalChecked()));
        return;
    }
    
    // Get the filename
    v8::String::Utf8Value filename(isolate, args[0]);
    
    // Check if this is a synchronous call (no callback)
    bool is_sync = args.Length() < 2 || !args[1]->IsFunction();
    
    if (is_sync) {
        // Synchronous read
        try {
            // Open the file
            std::ifstream file(*filename);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file");
            }
            
            // Read the file content
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            
            // Return the file content
            args.GetReturnValue().Set(
                v8::String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
        } catch (const std::exception& e) {
            // Handle errors
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, e.what()).ToLocalChecked()));
        }
    } else {
        // Asynchronous read (with callback)
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
            std::string content;
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
            data->buf = uv_buf_init(new char[4096], 4096);
            
            // Read the file
            uv_fs_read(req->loop, req, fd, &data->buf, 1, 0, [](uv_fs_t* req) {
                auto data = static_cast<FileReadData*>(req->data);
                
                if (req->result < 0) {
                    // Handle error
                    InvokeCallback(data->isolate, data->callback, 
                                   uv_strerror(req->result), nullptr);
                } else if (req->result == 0) {
                    // End of file, return the content
                    InvokeCallback(data->isolate, data->callback, nullptr, data->content.c_str());
                } else {
                    // Data read, accumulate it
                    data->content.append(data->buf.base, req->result);
                    
                    // Continue reading
                    uv_fs_read(req->loop, req, req->file, &data->buf, 1, data->content.length(), nullptr);
                    return;
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
1. Determines whether it's a synchronous or asynchronous call based on the presence of a callback function
2. For synchronous calls:
   - Opens and reads the file directly
   - Returns the content or throws an error
3. For asynchronous calls:
   - Uses LibUV to open and read the file asynchronously
   - Invokes the callback with the result when the operation completes
   - Properly handles errors and cleans up resources

### Writing Files

The `writeFile` function allows JavaScript code to write data to a file. Here's the implementation:

```cpp
void WriteFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Expected a filename and data string").ToLocalChecked()));
        return;
    }
    
    // Get the filename and data
    v8::String::Utf8Value filename(isolate, args[0]);
    v8::String::Utf8Value data(isolate, args[1]);
    
    // Check if this is a synchronous call (no callback)
    bool is_sync = args.Length() < 3 || !args[2]->IsFunction();
    
    if (is_sync) {
        // Synchronous write
        try {
            // Open the file
            std::ofstream file(*filename);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file for writing");
            }
            
            // Write the data
            file.write(*data, data.length());
            file.close();
            
            // Return undefined
            args.GetReturnValue().SetUndefined();
        } catch (const std::exception& e) {
            // Handle errors
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, e.what()).ToLocalChecked()));
        }
    } else {
        // Asynchronous write (with callback)
        // Get the callback function
        v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[2]);
        
        // Create a persistent handle to the callback
        auto persistent_callback = new v8::Persistent<v8::Function>(isolate, callback);
        
        // Get the event loop from the isolate data
        uv_loop_t* event_loop = static_cast<uv_loop_t*>(
            isolate->GetData(EVENT_LOOP_INDEX));
        
        // Create a structure to hold file data
        struct FileWriteData {
            uv_fs_t req;
            uv_buf_t buf;
            v8::Isolate* isolate;
            v8::Persistent<v8::Function>* callback;
            std::string data;
        };
        
        auto file_data = new FileWriteData;
        file_data->isolate = isolate;
        file_data->callback = persistent_callback;
        file_data->data = std::string(*data, data.length());
        
        // Open the file asynchronously
        file_data->req.data = file_data;
        uv_fs_open(event_loop, &file_data->req, *filename, O_WRONLY | O_CREAT | O_TRUNC, 0644, [](uv_fs_t* req) {
            auto data = static_cast<FileWriteData*>(req->data);
            
            if (req->result < 0) {
                // Handle error
                InvokeCallback(data->isolate, data->callback, 
                               uv_strerror(req->result), nullptr);
                delete data->callback;
                delete data;
                uv_fs_req_cleanup(req);
                return;
            }
            
            // File opened successfully, now write to it
            int fd = req->result;
            uv_fs_req_cleanup(req);
            
            // Create buffer for writing
            data->buf = uv_buf_init(const_cast<char*>(data->data.c_str()), data->data.length());
            
            // Write to the file
            uv_fs_write(req->loop, req, fd, &data->buf, 1, 0, [](uv_fs_t* req) {
                auto data = static_cast<FileWriteData*>(req->data);
                
                if (req->result < 0) {
                    // Handle error
                    InvokeCallback(data->isolate, data->callback, 
                                   uv_strerror(req->result), nullptr);
                } else {
                    // Write successful
                    InvokeCallback(data->isolate, data->callback, nullptr, nullptr);
                }
                
                // Close the file
                int fd = req->file;
                uv_fs_req_cleanup(req);
                uv_fs_close(req->loop, req, fd, [](uv_fs_t* req) {
                    auto data = static_cast<FileWriteData*>(req->data);
                    
                    // Clean up
                    delete data->callback;
                    delete data;
                    uv_fs_req_cleanup(req);
                });
            });
        });
    }
}
```

This function follows a similar pattern to `readFile`:
1. It determines whether it's a synchronous or asynchronous call
2. For synchronous calls, it directly opens, writes to, and closes the file
3. For asynchronous calls, it uses LibUV to handle the file operations asynchronously
4. It properly handles errors and cleans up resources

### Checking if Files Exist

The `exists` function allows JavaScript code to check if a file exists. Here's the implementation:

```cpp
void ExistsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Expected a filename string").ToLocalChecked()));
        return;
    }
    
    // Get the filename
    v8::String::Utf8Value filename(isolate, args[0]);
    
    // Check if this is a synchronous call (no callback)
    bool is_sync = args.Length() < 2 || !args[1]->IsFunction();
    
    if (is_sync) {
        // Synchronous check
        try {
            // Check if the file exists
            bool exists = std::filesystem::exists(*filename);
            
            // Return the result
            args.GetReturnValue().Set(exists);
        } catch (const std::exception& e) {
            // Handle errors
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, e.what()).ToLocalChecked()));
        }
    } else {
        // Asynchronous check (with callback)
        // Get the callback function
        v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[1]);
        
        // Create a persistent handle to the callback
        auto persistent_callback = new v8::Persistent<v8::Function>(isolate, callback);
        
        // Get the event loop from the isolate data
        uv_loop_t* event_loop = static_cast<uv_loop_t*>(
            isolate->GetData(EVENT_LOOP_INDEX));
        
        // Create a structure to hold data
        struct ExistsData {
            uv_fs_t req;
            v8::Isolate* isolate;
            v8::Persistent<v8::Function>* callback;
        };
        
        auto data = new ExistsData;
        data->isolate = isolate;
        data->callback = persistent_callback;
        
        // Check if the file exists asynchronously
        data->req.data = data;
        uv_fs_stat(event_loop, &data->req, *filename, [](uv_fs_t* req) {
            auto data = static_cast<ExistsData*>(req->data);
            
            // File exists if the result is >= 0
            bool exists = req->result >= 0;
            
            // Invoke the callback with the result
            v8::HandleScope handle_scope(data->isolate);
            v8::Local<v8::Context> context = data->isolate->GetCurrentContext();
            v8::Local<v8::Function> cb = v8::Local<v8::Function>::New(data->isolate, *data->callback);
            
            v8::Local<v8::Value> argv[] = { v8::Boolean::New(data->isolate, exists) };
            cb->Call(context, context->Global(), 1, argv).ToLocalChecked();
            
            // Clean up
            delete data->callback;
            delete data;
            uv_fs_req_cleanup(req);
        });
    }
}
```

This function:
1. Determines whether it's a synchronous or asynchronous call
2. For synchronous calls, it directly checks if the file exists using `std::filesystem`
3. For asynchronous calls, it uses LibUV's `uv_fs_stat` to check asynchronously
4. It properly handles errors and cleans up resources

## Using the FS Module in JavaScript

Once the fs module is implemented and registered, it can be used in JavaScript code like this:

```javascript
// Import the fs module
const fs = require('fs');

// Synchronous operations
try {
    // Check if a file exists
    if (fs.exists('file.txt')) {
        // Read the file
        const content = fs.readFile('file.txt');
        console.log('File content:', content);
        
        // Modify the content
        const newContent = content + '\nAppended data';
        
        // Write to the file
        fs.writeFile('file.txt', newContent);
        console.log('File updated successfully');
    } else {
        console.log('File does not exist');
    }
} catch (error) {
    console.error('Error:', error);
}

// Asynchronous operations
fs.exists('file.txt', (exists) => {
    if (exists) {
        fs.readFile('file.txt', (err, content) => {
            if (err) {
                console.error('Error reading file:', err);
                return;
            }
            
            console.log('File content:', content);
            
            const newContent = content + '\nAppended data';
            
            fs.writeFile('file.txt', newContent, (err) => {
                if (err) {
                    console.error('Error writing file:', err);
                    return;
                }
                
                console.log('File updated successfully');
            });
        });
    } else {
        console.log('File does not exist');
    }
});
```

This example demonstrates both synchronous and asynchronous usage of the fs module:
- Synchronous operations are simpler to write but block the JavaScript execution until they complete
- Asynchronous operations use callbacks and don't block execution, but they require a more complex, nested structure

## Error Handling

Proper error handling is crucial for file operations. In Tiny Node.js, errors are handled differently for synchronous and asynchronous operations:

1. **Synchronous operations**:
   - Errors are thrown as JavaScript exceptions using `isolate->ThrowException()`
   - These can be caught using try/catch blocks in JavaScript

2. **Asynchronous operations**:
   - Errors are passed as the first argument to the callback function
   - The callback pattern is `(error, result) => { ... }`
   - If there's no error, the first argument is `null` or `undefined`

This pattern is consistent with Node.js's error handling approach.

## Extending the FS Module

The fs module can be extended with additional functionality:

1. **Directory operations**: Creating, listing, and removing directories
2. **File information**: Getting file size, modification time, and other metadata
3. **File manipulation**: Renaming, moving, and deleting files
4. **Stream operations**: Reading and writing files using streams for efficient handling of large files
5. **Path utilities**: Normalizing, joining, and resolving paths

Each of these can be implemented following the same pattern as the existing functions, providing both synchronous and asynchronous versions.

## Performance Considerations

File operations can be performance-critical, especially in server applications. Here are some considerations:

1. **Asynchronous operations**: Prefer asynchronous operations for better performance and scalability
2. **Buffer management**: Efficiently allocate and reuse buffers for reading and writing
3. **Caching**: Consider caching frequently accessed files or metadata
4. **Error handling**: Properly handle errors to avoid resource leaks
5. **Memory management**: Clean up resources properly, especially in asynchronous operations

## Conclusion

The fs module is a fundamental component of any JavaScript runtime, providing access to the file system. By implementing both synchronous and asynchronous versions of key operations, Tiny Node.js provides a flexible and powerful interface for file system access.

Understanding the implementation of the fs module provides insights into how JavaScript runtimes bridge the gap between high-level JavaScript code and low-level system operations, and how asynchronous operations are handled using the event loop.

[← Previous: Event Loop](07-event-loop.md) | [Next: HTTP Server Implementation →](09-http-module.md) 