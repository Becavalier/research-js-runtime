# 4. Runtime Core Implementation

The Runtime class is the heart of Tiny Node.js, responsible for initializing the JavaScript environment, executing code, and managing the runtime lifecycle. This chapter explores its implementation in detail.

## The Runtime Class Overview

The Runtime class (`include/runtime.h` and `src/runtime.cpp`) serves as the central coordinator for our JavaScript runtime. Its responsibilities include:

- Initializing and managing the V8 engine
- Creating the JavaScript execution environment (context)
- Registering native functions and modules
- Executing JavaScript code
- Managing the event loop for asynchronous operations
- Providing cleanup and shutdown functionality

## Class Definition

Let's examine the Runtime class definition in `include/runtime.h`:

```cpp
class Runtime {
public:
    // Constructor and destructor
    Runtime(int argc, char* argv[]);
    ~Runtime();

    // Initialize V8 platform and engine
    static void InitializeV8(int argc, char* argv[]);
    static void ShutdownV8();

    // Execute a JavaScript file
    bool ExecuteFile(const std::string& filename);

    // Create a context for JavaScript execution
    v8::Local<v8::Context> CreateContext();

    // Register a native function
    void RegisterNativeFunction(const std::string& name, v8::FunctionCallback callback);

    // Register native modules (fs, http, etc.)
    void RegisterNativeModules();

    // Shutdown the runtime
    void Shutdown();

private:
    // V8 platform and isolate
    static std::unique_ptr<v8::Platform> platform_;
    v8::Isolate* isolate_;

    // Global object template
    v8::Global<v8::ObjectTemplate> global_template_;

    // Module system for handling require() calls
    std::unique_ptr<ModuleSystem> module_system_;

    // Event loop
    uv_loop_t* event_loop_;

    // Command line arguments
    std::vector<std::string> args_;

    // Map of native function names to callbacks
    std::unordered_map<std::string, v8::FunctionCallback> native_functions_;

    // Helper methods
    bool ExecuteString(const std::string& source, const std::string& name);
};
```

## Initialization and Setup

### Constructor

The constructor initializes the Runtime instance, setting up the V8 environment and necessary components:

```cpp
Runtime::Runtime(int argc, char* argv[]) : isolate_(nullptr), event_loop_(nullptr) {
    std::cout << "Runtime constructor: Creating isolate..." << std::endl;
    
    // Store command line arguments
    for (int i = 0; i < argc; i++) {
        args_.push_back(argv[i]);
    }

    // Create a new isolate with its own heap
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate_ = v8::Isolate::New(create_params);
    std::cout << "Runtime constructor: Isolate created" << std::endl;

    // Create a global template for the JavaScript environment
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    
    std::cout << "Runtime constructor: Creating global template..." << std::endl;
    v8::Local<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New(isolate_);
    global_template_.Reset(isolate_, global_template);

    // Create the module system
    std::cout << "Runtime constructor: Creating module system..." << std::endl;
    module_system_ = std::make_unique<ModuleSystem>(isolate_);

    // Set up global functions
    std::cout << "Runtime constructor: Setting up global functions..." << std::endl;
    RegisterNativeFunction("print", PrintCallback);
    RegisterNativeFunction("setTimeout", SetTimeoutCallback);
    RegisterNativeFunction("clearTimeout", ClearTimeoutCallback);
    RegisterNativeFunction("require", RequireCallback);

    // Register native modules
    std::cout << "Runtime constructor: Registering native modules..." << std::endl;
    RegisterNativeModules();

    // Create event loop for asynchronous operations
    std::cout << "Runtime constructor: Creating event loop..." << std::endl;
    event_loop_ = new uv_loop_t;
    uv_loop_init(event_loop_);
    
    std::cout << "Runtime constructor: Complete" << std::endl;
}
```

Key aspects of the constructor:

1. **Isolate Creation**: Creates a new V8 isolate, which is an independent JavaScript runtime environment.
2. **Global Template**: Sets up a template for the global object that will be used in each context.
3. **Module System**: Initializes the module system for handling `require()` calls.
4. **Native Functions**: Registers built-in JavaScript functions implemented in C++.
5. **Native Modules**: Registers modules like `fs` and `http`.
6. **Event Loop**: Creates the event loop using LibUV for handling asynchronous operations.

### Static V8 Initialization

Before creating a Runtime instance, you must initialize V8. This is done using the static `InitializeV8` method:

```cpp
void Runtime::InitializeV8(int argc, char* argv[]) {
    // Initialize V8
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    
    // Create the V8 platform
    platform_ = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform_.get());
    
    // Initialize V8 engine
    v8::V8::Initialize();
}
```

This method sets up the V8 engine, which includes:
- Initializing the International Components for Unicode (ICU)
- Setting up external startup data
- Creating the V8 platform
- Initializing the V8 engine itself

## Creating the JavaScript Environment

### Context Creation

The `CreateContext` method sets up a JavaScript execution environment:

```cpp
v8::Local<v8::Context> Runtime::CreateContext() {
    std::cout << "CreateContext: Starting..." << std::endl;
    
    v8::EscapableHandleScope handle_scope(isolate_);
    
    // Create a local handle for the global template
    v8::Local<v8::ObjectTemplate> global_template = 
        v8::Local<v8::ObjectTemplate>::New(isolate_, global_template_);
    
    // Create a new context
    v8::Local<v8::Context> context = v8::Context::New(isolate_, nullptr, global_template);
    
    // Enter the context
    v8::Context::Scope context_scope(context);
    
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
    
    // Register the process object as global
    RegisterProcessModule(isolate_, context);
    
    std::cout << "CreateContext: Complete" << std::endl;
    return handle_scope.Escape(context);
}
```

This method:
1. Creates a new context based on the global template
2. Adds all registered native functions to the global object
3. Registers the `process` object as a global variable
4. Returns the context for JavaScript execution

## Executing JavaScript

### File Execution

The `ExecuteFile` method loads and executes a JavaScript file:

```cpp
bool Runtime::ExecuteFile(const std::string& filename) {
    std::cout << "Executing file: " << filename << std::endl;
    
    // Read the file content
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Execute the file content
    bool result = ExecuteString(source, filename);
    
    if (result) {
        std::cout << "File executed successfully, waiting for event loop..." << std::endl;
        
        // Run the event loop until it's empty
        uv_run(event_loop_, UV_RUN_DEFAULT);
    } else {
        std::cerr << "Failed to execute file: " << filename << std::endl;
    }
    
    return result;
}
```

This method:
1. Reads the content of the JavaScript file
2. Executes the content using the `ExecuteString` method
3. If successful, runs the event loop to process any asynchronous operations
4. Returns whether the execution was successful

### String Execution

The `ExecuteString` method compiles and runs JavaScript code:

```cpp
bool Runtime::ExecuteString(const std::string& source, const std::string& name) {
    std::cout << "ExecuteString: Starting..." << std::endl;
    
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    
    // Create a context for execution
    v8::Local<v8::Context> context = CreateContext();
    v8::Context::Scope context_scope(context);
    
    // Create a string containing the JavaScript source code
    v8::Local<v8::String> source_string = 
        v8::String::NewFromUtf8(isolate_, source.c_str()).ToLocalChecked();
        
    // Create a string for the file name
    v8::Local<v8::String> name_string = 
        v8::String::NewFromUtf8(isolate_, name.c_str()).ToLocalChecked();
    
    // Set up error handling
    v8::TryCatch try_catch(isolate_);
    
    // Compile the script
    std::cout << "ExecuteString: Compiling script..." << std::endl;
    v8::ScriptOrigin origin(isolate_, name_string);
    v8::Local<v8::Script> script;
    
    if (!v8::Script::Compile(context, source_string, &origin).ToLocal(&script)) {
        // Report compilation errors
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Compilation error: " << *error << std::endl;
        return false;
    }
    
    std::cout << "ExecuteString: Script compiled successfully" << std::endl;
    std::cout << "ExecuteString: Running script..." << std::endl;
    
    // Run the script
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
        // Report execution errors
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Execution error: " << *error << std::endl;
        return false;
    }
    
    std::cout << "ExecuteString: Script executed successfully" << std::endl;
    
    // Print the result if it's not undefined
    if (!result->IsUndefined()) {
        v8::String::Utf8Value str(isolate_, result);
        std::cout << "Script result: " << *str << std::endl;
    }
    
    std::cout << "ExecuteString: Complete" << std::endl;
    return true;
}
```

This method:
1. Creates a context for execution
2. Compiles the JavaScript code into a script
3. Executes the script
4. Handles and reports any errors that occur
5. Returns whether the execution was successful

## Registering Native Functions and Modules

### Native Function Registration

The `RegisterNativeFunction` method makes C++ functions available to JavaScript:

```cpp
void Runtime::RegisterNativeFunction(const std::string& name, v8::FunctionCallback callback) {
    std::cout << "RegisterNativeFunction: Starting for " << name << "..." << std::endl;
    
    // Store the function for later use when creating contexts
    native_functions_[name] = callback;
    
    std::cout << "RegisterNativeFunction: Complete for " << name << std::endl;
}
```

This method stores the function name and callback for later use when creating contexts.

### Native Module Registration

The `RegisterNativeModules` method sets up built-in modules:

```cpp
void Runtime::RegisterNativeModules() {
    std::cout << "RegisterNativeModules: Starting..." << std::endl;
    
    // Register built-in modules
    RegisterFsModule(isolate_, module_system_.get());
    RegisterHttpModule(isolate_, module_system_.get());
    
    std::cout << "RegisterNativeModules: Complete" << std::endl;
}
```

This method registers the built-in modules like `fs` and `http` with the module system.

## Cleanup and Shutdown

### Destructor

The destructor cleans up resources:

```cpp
Runtime::~Runtime() {
    if (event_loop_) {
        uv_loop_close(event_loop_);
        delete event_loop_;
    }
    
    global_template_.Reset();
    
    if (isolate_) {
        isolate_->Dispose();
    }
}
```

This method:
1. Closes and deletes the event loop
2. Releases the global template
3. Disposes of the isolate

### Runtime Shutdown

The `Shutdown` method provides a controlled way to shut down the runtime:

```cpp
void Runtime::Shutdown() {
    std::cout << "Shutdown: Starting..." << std::endl;
    
    // Clean up the event loop
    uv_loop_close(event_loop_);
    delete event_loop_;
    event_loop_ = nullptr;
    
    std::cout << "Shutdown: Complete" << std::endl;
}
```

This method cleans up the event loop and prepares the runtime for shutdown.

### Static V8 Shutdown

The static `ShutdownV8` method shuts down the V8 engine:

```cpp
void Runtime::ShutdownV8() {
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    platform_.reset();
}
```

This method:
1. Disposes of the V8 engine
2. Shuts down the V8 platform
3. Releases the platform

## The Main Function

The main function (`src/main.cpp`) ties everything together:

```cpp
int main(int argc, char* argv[]) {
    std::cout << "Starting main function..." << std::endl;
    
    // Check if a JavaScript file was provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <javascript_file>" << std::endl;
        return 1;
    }
    
    std::cout << "Initializing runtime..." << std::endl;
    
    // Initialize V8
    std::cout << "Initializing V8..." << std::endl;
    Runtime::InitializeV8(argc, argv);
    
    {
        // Create a runtime instance
        std::cout << "Creating runtime instance..." << std::endl;
        Runtime runtime(argc, argv);
        
        // Execute the JavaScript file
        const char* filename = argv[1];
        if (!runtime.ExecuteFile(filename)) {
            return 1;
        }
        
        // Shutdown the runtime
        std::cout << "Shutting down runtime..." << std::endl;
        runtime.Shutdown();
    }
    
    // Shutdown V8
    Runtime::ShutdownV8();
    
    return 0;
}
```

This function:
1. Checks if a JavaScript file was provided
2. Initializes the V8 engine
3. Creates a Runtime instance
4. Executes the specified JavaScript file
5. Shuts down the runtime
6. Shuts down the V8 engine

## Key Design Considerations

### Memory Management

The Runtime class carefully manages memory to prevent leaks:
- Uses handle scopes to manage local handles
- Properly disposes of the isolate
- Cleans up the event loop

### Error Handling

Error handling is implemented throughout:
- Compilation errors are caught and reported
- Execution errors are caught and reported
- Resources are properly cleaned up even when errors occur

### Modularity

The Runtime class is designed to be modular:
- Separates V8 initialization from runtime creation
- Uses a dedicated Module System for handling modules
- Registers native functions and modules independently

## Advanced Usage

### Adding New Native Functions

To add a new native function:

1. Define a callback function that follows the V8 function callback signature:
```cpp
void MyFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Implementation
}
```

2. Register the function with the Runtime:
```cpp
runtime.RegisterNativeFunction("myFunction", MyFunctionCallback);
```

### Adding New Native Modules

To add a new native module:

1. Create a registration function that follows the module registration pattern:
```cpp
void RegisterMyModule(v8::Isolate* isolate, ModuleSystem* module_system) {
    // Implementation
}
```

2. Update the `RegisterNativeModules` method to include your new module:
```cpp
void Runtime::RegisterNativeModules() {
    // Existing modules
    RegisterFsModule(isolate_, module_system_.get());
    RegisterHttpModule(isolate_, module_system_.get());
    
    // New module
    RegisterMyModule(isolate_, module_system_.get());
}
```

## Conclusion

The Runtime class is the heart of Tiny Node.js, providing the core functionality needed to execute JavaScript code outside of a browser. It initializes the V8 engine, creates the execution environment, registers native functions and modules, and manages the event loop for asynchronous operations.

Understanding this class is key to understanding how the entire runtime works, as it ties together all the other components into a functioning JavaScript runtime.

[← Previous: Project Structure](03-project-structure.md) | [Next: JavaScript Native Functions →](05-native-functions.md) 