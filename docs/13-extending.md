# 13. Building Your Own Features

One of the strengths of a JavaScript runtime is its extensibility. This chapter will guide you through the process of extending Tiny Node.js with new features, native modules, and functionality. Whether you want to add support for new APIs, integrate with system libraries, or implement custom functionality, this chapter will provide you with the knowledge and tools to do so.

## Why Extend the Runtime?

There are several reasons why you might want to extend a JavaScript runtime:

1. **Adding Missing Functionality**: Implementing features that aren't included in the core runtime
2. **Performance Optimization**: Moving performance-critical code to C++ for better efficiency
3. **System Integration**: Providing access to operating system features or hardware
4. **Domain-Specific Features**: Adding specialized functionality for particular use cases
5. **Experimentation**: Trying out new ideas or concepts before they become standardized

By extending the runtime, you can make it more suitable for your specific needs while maintaining the flexibility and ease of use of JavaScript.

## Types of Extensions

There are several ways to extend Tiny Node.js:

1. **Native Functions**: Adding new global functions implemented in C++
2. **Native Modules**: Creating new modules implemented in C++
3. **Core Modules**: Implementing modules in JavaScript that use native functionality
4. **Custom Features**: Adding entirely new subsystems or capabilities

Let's explore each of these approaches.

## Adding Native Functions

Native functions are JavaScript functions implemented in C++. They're typically exposed globally and provide core functionality that can't be implemented efficiently in JavaScript alone.

### Step 1: Define the Function Callback

Start by defining a function callback that follows the V8 signature:

```cpp
// In a header file (e.g., include/my_functions.h)
#pragma once

#include <v8.h>

// Declaration of the function callback
void MyFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
```

```cpp
// In a source file (e.g., src/my_functions.cpp)
#include "my_functions.h"
#include <iostream>

void MyFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    
    // Get the context
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Process the argument
    v8::String::Utf8Value str(isolate, args[0]);
    std::string result = "Hello, " + std::string(*str) + "!";
    
    // Return the result
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}
```

This function takes a string argument, prepends "Hello, " to it, appends "!", and returns the result.

### Step 2: Register the Function with the Runtime

Next, register the function with the runtime by adding it to the `Runtime` class's constructor:

```cpp
// In src/runtime.cpp, add to the constructor
RegisterNativeFunction("greet", MyFunctionCallback);
```

Don't forget to include your header file:

```cpp
// Near the top of src/runtime.cpp
#include "my_functions.h"
```

### Step 3: Update the Build Configuration

Add your new source file to the build configuration:

```cmake
# In CMakeLists.txt, update the sources list
file(GLOB SOURCES 
    "src/*.cpp"
    "src/modules/*.cpp"
)
```

### Step 4: Use the Function in JavaScript

Now you can use your new function in JavaScript:

```javascript
// In a JavaScript file
const greeting = greet("World");
print(greeting);  // Outputs: Hello, World!
```

## Creating Native Modules

Native modules are collections of related functionality implemented in C++ and exposed to JavaScript as a module. They provide a way to organize and namespace related functionality.

### Step 1: Define the Module Structure

Start by defining the structure of your module:

```cpp
// In include/modules/my_module.h
#pragma once

#include <v8.h>
#include "module_system.h"

// Function to register the module
void RegisterMyModule(v8::Isolate* isolate, ModuleSystem* module_system);

// Module function callbacks
void MyModuleFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
```

```cpp
// In src/modules/my_module.cpp
#include "modules/my_module.h"
#include <iostream>

void RegisterMyModule(v8::Isolate* isolate, ModuleSystem* module_system) {
    std::cout << "RegisterMyModule: Starting..." << std::endl;
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    
    // Create and enter a context
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    
    // Create the module object
    v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
    
    // Add functions to the module
    module_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "doSomething").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, MyModuleFunctionCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Register the module
    module_system->RegisterNativeModule("myModule", module_obj);
    
    std::cout << "RegisterMyModule: Complete" << std::endl;
}

void MyModuleFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    
    // Function implementation
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, "Module function called!").ToLocalChecked());
}
```

This module exports a function called `doSomething` that returns a string.

### Step 2: Register the Module with the Runtime

Next, register the module with the runtime by adding it to the `RegisterNativeModules` method:

```cpp
// In src/runtime.cpp, add to RegisterNativeModules
void Runtime::RegisterNativeModules() {
    std::cout << "RegisterNativeModules: Starting..." << std::endl;
    
    // Register existing modules
    RegisterFsModule(isolate_, module_system_.get());
    RegisterHttpModule(isolate_, module_system_.get());
    
    // Register the new module
    RegisterMyModule(isolate_, module_system_.get());
    
    std::cout << "RegisterNativeModules: Complete" << std::endl;
}
```

Don't forget to include your header file:

```cpp
// Near the top of src/runtime.cpp
#include "modules/my_module.h"
```

### Step 3: Update the Build Configuration

Add your new source file to the build configuration:

```cmake
# In CMakeLists.txt, update the include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/modules
    ${V8_INCLUDE_DIR}
    ${LIBUV_INCLUDE_DIRS}
)
```

### Step 4: Use the Module in JavaScript

Now you can use your new module in JavaScript:

```javascript
// In a JavaScript file
const myModule = require('myModule');
const result = myModule.doSomething();
print(result);  // Outputs: Module function called!
```

## Implementing Core Modules in JavaScript

Core modules implemented in JavaScript provide a way to create higher-level abstractions on top of native functionality. They allow for more flexibility and easier updates than native modules.

### Step 1: Create the JavaScript Module

Create a JavaScript file for your module:

```javascript
// In lib/my_module.js
// Module dependencies
const fs = require('fs');

// Module functionality
function readAndProcess(filename) {
    const content = fs.readFile(filename);
    return process(content);
}

function process(content) {
    return content.toUpperCase();
}

// Export the module interface
module.exports = {
    readAndProcess
};
```

This module provides a function to read a file and convert its content to uppercase.

### Step 2: Register the Module Path

Update the module system to know where to find your module:

```cpp
// In src/module_system.cpp, add to the ResolveModulePath method
std::string ModuleSystem::ResolveModulePath(const std::string& module_name) {
    // Check if it's a built-in module
    if (module_name == "myModule") {
        return "lib/my_module.js";
    }
    
    // Existing code for other modules
    // ...
}
```

### Step 3: Use the Module in JavaScript

Now you can use your new module in JavaScript:

```javascript
// In a JavaScript file
const myModule = require('myModule');
const result = myModule.readAndProcess('test.txt');
print(result);  // Outputs the file content in uppercase
```

## Adding a New Subsystem

Adding a new subsystem involves creating a set of related native functions and potentially modules that work together to provide a new capability.

### Example: Adding WebSocket Support

Let's walk through a simplified example of adding WebSocket support to Tiny Node.js.

### Step 1: Define the WebSocket Module

```cpp
// In include/modules/websocket_module.h
#pragma once

#include <v8.h>
#include "module_system.h"

// Function to register the module
void RegisterWebSocketModule(v8::Isolate* isolate, ModuleSystem* module_system);

// WebSocket constructor callback
void WebSocketConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

// WebSocket method callbacks
void WebSocketSendCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebSocketCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
```

```cpp
// In src/modules/websocket_module.cpp
#include "modules/websocket_module.h"
#include <iostream>
#include <uv.h>

// WebSocket implementation (simplified)
struct WebSocketData {
    uv_tcp_t socket;
    v8::Persistent<v8::Object> js_object;
    v8::Isolate* isolate;
    // Other WebSocket state
};

void RegisterWebSocketModule(v8::Isolate* isolate, ModuleSystem* module_system) {
    std::cout << "RegisterWebSocketModule: Starting..." << std::endl;
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    
    // Create and enter a context
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    
    // Create the WebSocket constructor
    v8::Local<v8::FunctionTemplate> constructor_template = 
        v8::FunctionTemplate::New(isolate, WebSocketConstructorCallback);
    constructor_template->SetClassName(
        v8::String::NewFromUtf8(isolate, "WebSocket").ToLocalChecked());
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    
    // Add methods to the prototype
    v8::Local<v8::ObjectTemplate> prototype_template = 
        constructor_template->PrototypeTemplate();
    prototype_template->Set(
        isolate,
        "send",
        v8::FunctionTemplate::New(isolate, WebSocketSendCallback)
    );
    prototype_template->Set(
        isolate,
        "close",
        v8::FunctionTemplate::New(isolate, WebSocketCloseCallback)
    );
    
    // Create the module object
    v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
    
    // Add the WebSocket constructor to the module
    module_obj->Set(
        context,
        v8::String::NewFromUtf8(isolate, "WebSocket").ToLocalChecked(),
        constructor_template->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Register the module
    module_system->RegisterNativeModule("websocket", module_obj);
    
    std::cout << "RegisterWebSocketModule: Complete" << std::endl;
}

void WebSocketConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    // Check if it's a constructor call
    if (!args.IsConstructCall()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "WebSocket constructor requires 'new'").ToLocalChecked()));
        return;
    }
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "WebSocket constructor requires a URL").ToLocalChecked()));
        return;
    }
    
    // Get the URL
    v8::String::Utf8Value url(isolate, args[0]);
    
    // Create WebSocket data
    WebSocketData* data = new WebSocketData();
    data->isolate = isolate;
    
    // Store the JavaScript object
    v8::Local<v8::Object> js_object = args.This();
    data->js_object.Reset(isolate, js_object);
    
    // Store the data in the JavaScript object
    js_object->SetInternalField(0, v8::External::New(isolate, data));
    
    // Initialize the socket
    uv_loop_t* loop = static_cast<uv_loop_t*>(
        isolate->GetData(RUNTIME_EVENT_LOOP_INDEX));
    uv_tcp_init(loop, &data->socket);
    data->socket.data = data;
    
    // Connect and set up WebSocket handshake (simplified)
    // ...
    
    // Return the WebSocket object
    args.GetReturnValue().Set(js_object);
}

void WebSocketSendCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    
    // Get the WebSocket data
    v8::Local<v8::Object> js_object = args.This();
    v8::Local<v8::Value> external = js_object->GetInternalField(0);
    WebSocketData* data = static_cast<WebSocketData*>(
        v8::External::Cast(*external)->Value());
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "send requires a string message").ToLocalChecked()));
        return;
    }
    
    // Get the message
    v8::String::Utf8Value message(isolate, args[0]);
    
    // Send the message (simplified)
    // ...
    
    // Return undefined
    args.GetReturnValue().SetUndefined();
}

void WebSocketCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    
    // Get the WebSocket data
    v8::Local<v8::Object> js_object = args.This();
    v8::Local<v8::Value> external = js_object->GetInternalField(0);
    WebSocketData* data = static_cast<WebSocketData*>(
        v8::External::Cast(*external)->Value());
    
    // Close the socket (simplified)
    uv_close(reinterpret_cast<uv_handle_t*>(&data->socket), [](uv_handle_t* handle) {
        WebSocketData* data = static_cast<WebSocketData*>(handle->data);
        data->js_object.Reset();
        delete data;
    });
    
    // Return undefined
    args.GetReturnValue().SetUndefined();
}
```

This implementation provides a simple WebSocket class with `send` and `close` methods. It's a significant simplification of what would be needed for a real WebSocket implementation, but it illustrates the general approach.

### Step 2: Register the WebSocket Module

```cpp
// In src/runtime.cpp, add to RegisterNativeModules
void Runtime::RegisterNativeModules() {
    std::cout << "RegisterNativeModules: Starting..." << std::endl;
    
    // Register existing modules
    RegisterFsModule(isolate_, module_system_.get());
    RegisterHttpModule(isolate_, module_system_.get());
    
    // Register the WebSocket module
    RegisterWebSocketModule(isolate_, module_system_.get());
    
    std::cout << "RegisterNativeModules: Complete" << std::endl;
}
```

### Step 3: Use the WebSocket in JavaScript

```javascript
// In a JavaScript file
const { WebSocket } = require('websocket');

// Create a WebSocket connection
const ws = new WebSocket('ws://example.com/socket');

// Set up event listeners
ws.onopen = () => {
    print('WebSocket connected');
    ws.send('Hello, server!');
};

ws.onmessage = (event) => {
    print('Received:', event.data);
};

ws.onclose = () => {
    print('WebSocket closed');
};

// Later, close the connection
setTimeout(() => {
    ws.close();
}, 5000);
```

## Best Practices for Extending the Runtime

When extending Tiny Node.js, follow these best practices:

### 1. Maintain JavaScript Semantics

Ensure that your extensions follow JavaScript's semantics and conventions:

- Return `undefined` when a function doesn't explicitly return a value
- Throw appropriate exceptions for errors
- Follow JavaScript naming conventions (camelCase for functions and properties)
- Implement standard JavaScript patterns like callbacks, promises, or event emitters

### 2. Handle Memory Properly

Proper memory management is crucial:

- Use handle scopes to manage local handles
- Reset and delete persistent handles when they're no longer needed
- Clean up native resources when corresponding JavaScript objects are garbage collected
- Use weak handles for large or resource-intensive objects

### 3. Provide Clear Error Messages

Clear error messages help developers understand and fix issues:

- Include the expected and actual types in type errors
- Provide context in error messages
- Reference relevant documentation when appropriate
- Include the source of the error (e.g., file path, function name)

### 4. Document Your Extensions

Good documentation is essential for usability:

- Document the purpose and usage of each function and module
- Include examples of how to use the functionality
- Document expected arguments and return values
- Document error conditions and thrown exceptions

### 5. Write Tests

Tests ensure that your extensions work correctly:

- Write tests for normal usage
- Write tests for edge cases and error conditions
- Test memory management to ensure there are no leaks
- Test performance for critical operations

### 6. Maintain Compatibility

Ensure compatibility with the rest of the runtime:

- Follow the existing patterns and conventions
- Avoid modifying global state in ways that could affect other modules
- Test your extensions with existing functionality
- Consider backward compatibility for future updates

## Conclusion

Extending Tiny Node.js allows you to add new functionality, optimize performance, and integrate with system features. By adding native functions, creating native modules, implementing core modules in JavaScript, or adding entirely new subsystems, you can customize the runtime to meet your specific needs.

The examples and best practices provided in this chapter should give you a solid foundation for extending Tiny Node.js in a clean, maintainable, and efficient way. As you build on this foundation, you'll gain deeper insights into how JavaScript runtimes work and how to create powerful, specialized tools for your applications.

[← Previous: Advanced Topics](12-advanced-topics.md) 