# 5. JavaScript Native Functions

Native functions are a critical component of any JavaScript runtime, providing the bridge between JavaScript code and the underlying system. This chapter explores how Tiny Node.js implements native functions and exposes C++ functionality to JavaScript.

## What Are Native Functions?

Native functions in Tiny Node.js are JavaScript functions implemented in C++. They allow JavaScript code to interact with the operating system, perform I/O operations, access system resources, and execute operations that wouldn't be possible in pure JavaScript.

Examples of native functions in our runtime include:
- `print()`: For outputting text to the console
- `setTimeout()`: For scheduling delayed execution
- `clearTimeout()`: For canceling scheduled timeouts
- `require()`: For loading modules

## The V8 Function Callback Mechanism

V8 provides a mechanism for implementing JavaScript functions in C++ through function callbacks. The key type here is `v8::FunctionCallback`, which is defined as:

```cpp
typedef void (*FunctionCallback)(const FunctionCallbackInfo<Value>& info);
```

This is a function pointer that takes a `FunctionCallbackInfo` object containing information about the function call, including arguments, the calling context, and the return value.

### The FunctionCallbackInfo Structure

The `FunctionCallbackInfo` template provides all the information about a function call:

```cpp
template <typename T>
class FunctionCallbackInfo {
public:
    // Number of arguments passed
    int Length() const;
    
    // Get an argument at the specified index
    Local<Value> operator[](int i) const;
    
    // Get the receiver (usually 'this')
    Local<Object> This() const;
    
    // Get the holder of the function
    Local<Object> Holder() const;
    
    // Get the isolate
    Isolate* GetIsolate() const;
    
    // Set the return value
    void GetReturnValue().Set(Local<Value> value);
};
```

## Implementing Native Functions

Let's look at how native functions are implemented in Tiny Node.js.

### The Print Function

The `print` function is a simple native function that outputs text to the console:

```cpp
void PrintCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Create a string buffer
    std::string result;
    
    // Convert each argument to a string and append it
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(isolate);
        
        if (i > 0) {
            result += " ";
        }
        
        v8::String::Utf8Value str(isolate, args[i]);
        result += *str;
    }
    
    // Output the result to stdout
    std::cout << result << std::endl;
}
```

Key aspects of this implementation:
1. It accesses the arguments passed to the function using `args[i]`
2. It converts each argument to a string using `v8::String::Utf8Value`
3. It outputs the combined string to the console

### The setTimeout Function

The `setTimeout` function schedules a function to run after a specified delay:

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

Key aspects of this implementation:
1. It validates the arguments to ensure the first is a function and the second is a number
2. It creates a persistent handle to the callback function to keep it alive until it's called
3. It uses LibUV to create a timer for the specified delay
4. It sets up a callback to execute the JavaScript function when the timer expires
5. It cleans up resources when the timeout is executed

### The clearTimeout Function

The `clearTimeout` function cancels a previously scheduled timeout:

```cpp
void ClearTimeoutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsExternal()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the timer handle
    uv_timer_t* timer = static_cast<uv_timer_t*>(
        v8::External::Cast(*args[0])->Value());
    
    // Get the persistent callback
    auto persistent_callback = static_cast<v8::Persistent<v8::Function>*>(timer->data);
    
    // Clean up
    persistent_callback->Reset();
    delete persistent_callback;
    
    // Stop the timer
    uv_timer_stop(timer);
    uv_close(reinterpret_cast<uv_handle_t*>(timer), [](uv_handle_t* handle) {
        delete reinterpret_cast<uv_timer_t*>(handle);
    });
}
```

Key aspects of this implementation:
1. It validates that the argument is an external value (the timer handle)
2. It stops the timer and frees the associated resources

### The require Function

The `require` function loads and returns a module:

```cpp
void RequireCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate
    v8::Isolate* isolate = args.GetIsolate();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the module name
    v8::String::Utf8Value module_name(isolate, args[0]);
    
    // Get the module system from the isolate data
    ModuleSystem* module_system = static_cast<ModuleSystem*>(
        isolate->GetData(MODULE_SYSTEM_INDEX));
    
    // Load the module
    v8::Local<v8::Value> exports = module_system->Require(*module_name);
    
    // Return the module exports
    args.GetReturnValue().Set(exports);
}
```

Key aspects of this implementation:
1. It validates that the argument is a string (the module name)
2. It delegates to the ModuleSystem to load the module
3. It returns the module's exports

## Registering Native Functions

Native functions are registered with the runtime using the `RegisterNativeFunction` method:

```cpp
void Runtime::RegisterNativeFunction(const std::string& name, v8::FunctionCallback callback) {
    std::cout << "RegisterNativeFunction: Starting for " << name << "..." << std::endl;
    
    // Store the function for later use when creating contexts
    native_functions_[name] = callback;
    
    std::cout << "RegisterNativeFunction: Complete for " << name << std::endl;
}
```

This method stores the function name and callback in a map for later use when creating contexts.

## Adding Native Functions to the Context

When a context is created, all registered native functions are added to the global object:

```cpp
v8::Local<v8::Context> Runtime::CreateContext() {
    // ... (earlier code)
    
    // Add native functions to the context
    std::cout << "CreateContext: Adding native functions to context..." << std::endl;
    v8::Local<v8::Object> global = context->Global();
    
    for (const auto& [name, callback] : native_functions_) {
        std::cout << "CreateContext: Adding function " << name << " to context" << std::endl;
        
        v8::Local<v8::FunctionTemplate> function_template = 
            v8::FunctionTemplate::New(isolate_, callback);
            
        global->Set(
            context,
            v8::String::NewFromUtf8(isolate_, name.c_str()).ToLocalChecked(),
            function_template->GetFunction(context).ToLocalChecked()
        ).Check();
    }
    
    // ... (later code)
}
```

This code:
1. Iterates through all registered native functions
2. Creates a function template for each function
3. Adds the function to the global object in the context

## Best Practices for Native Functions

When implementing native functions, follow these best practices to ensure reliability and performance:

### 1. Validate Arguments

Always validate the arguments passed to your function before using them:

```cpp
if (args.Length() < 1 || !args[0]->IsString()) {
    isolate->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
    return;
}
```

### 2. Use HandleScope

Always use a HandleScope when creating local handles to manage memory correctly:

```cpp
v8::HandleScope handle_scope(isolate);
```

### 3. Properly Handle Errors

Throw exceptions when errors occur to provide meaningful feedback to JavaScript code:

```cpp
isolate->ThrowException(v8::Exception::Error(
    v8::String::NewFromUtf8(isolate, "Error message").ToLocalChecked()));
```

### 4. Manage Memory for Persistent Handles

When creating persistent handles, remember to free them when they're no longer needed:

```cpp
// Create a persistent handle
auto persistent = new v8::Persistent<v8::Object>(isolate, obj);

// When done with it
persistent->Reset();
delete persistent;
```

### 5. Consider Threading

Remember that V8 is not thread-safe. If your native function performs operations in other threads, you must use appropriate synchronization:

```cpp
// Wrong: Calling V8 from another thread
std::thread([isolate, callback]() {
    callback->Call(...);  // Will crash!
}).detach();

// Correct: Use the event loop to get back to the V8 thread
std::thread([loop]() {
    // Do work in this thread
    
    // Schedule a callback on the event loop
    uv_async_send(async_handle);
}).detach();
```

## Common Patterns for Native Functions

### Converting JavaScript Values to C++

```cpp
// String
v8::String::Utf8Value str(isolate, args[0]);
std::string cpp_str(*str);

// Number
int32_t integer = args[0]->Int32Value(context).FromJust();
double number = args[0]->NumberValue(context).FromJust();

// Boolean
bool boolean = args[0]->BooleanValue(isolate);

// Array
v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args[0]);
int length = array->Length();
```

### Converting C++ Values to JavaScript

```cpp
// String
args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, "hello").ToLocalChecked());

// Number
args.GetReturnValue().Set(v8::Number::New(isolate, 42));

// Boolean
args.GetReturnValue().Set(v8::Boolean::New(isolate, true));

// Array
v8::Local<v8::Array> array = v8::Array::New(isolate, 3);
array->Set(context, 0, v8::Number::New(isolate, 1)).Check();
array->Set(context, 1, v8::Number::New(isolate, 2)).Check();
array->Set(context, 2, v8::Number::New(isolate, 3)).Check();
args.GetReturnValue().Set(array);
```

### Creating JavaScript Objects

```cpp
v8::Local<v8::Object> obj = v8::Object::New(isolate);
obj->Set(
    context,
    v8::String::NewFromUtf8(isolate, "property").ToLocalChecked(),
    v8::Number::New(isolate, 42)
).Check();
```

### Calling JavaScript Functions from C++

```cpp
v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(args[0]);
v8::Local<v8::Value> argv[1] = { v8::String::NewFromUtf8(isolate, "argument").ToLocalChecked() };
func->Call(context, context->Global(), 1, argv).ToLocalChecked();
```

## Example: A Complete Native Function

Let's put it all together with a complete example of a native function that reads a file synchronously:

```cpp
void ReadFileSyncCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the isolate and context
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Expected a filename string").ToLocalChecked()));
        return;
    }
    
    // Get the filename
    v8::String::Utf8Value filename(isolate, args[0]);
    
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
}
```

This function:
1. Validates that it received a string argument
2. Converts the JavaScript string to a C++ string
3. Opens and reads the file
4. Returns the file content as a JavaScript string
5. Handles errors by throwing JavaScript exceptions

## Conclusion

Native functions are the bridge between JavaScript and the underlying system in Tiny Node.js. They allow JavaScript code to interact with the operating system, perform I/O operations, and access system resources. By understanding how to implement and register native functions, you can extend the functionality of the runtime to meet your needs.

In the next chapter, we'll explore the module system, which allows JavaScript code to be organized into reusable modules.

[← Previous: Runtime Core Implementation](04-runtime-core.md) | [Next: Module System →](06-module-system.md) 