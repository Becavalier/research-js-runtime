# 6. Module System

The module system is a fundamental part of any JavaScript runtime, allowing code to be organized into reusable, maintainable units. This chapter explores how Tiny Node.js implements its module system, which is inspired by Node.js's CommonJS module format.

## What is a Module System?

A module system allows JavaScript code to be split into separate files (modules) that can import functionality from each other. It provides several benefits:

- **Code organization**: Break down complex applications into manageable pieces
- **Encapsulation**: Keep internal details private within a module
- **Reusability**: Use the same module in multiple parts of an application
- **Dependency management**: Clearly define relationships between different parts of code

## CommonJS Module Format

Tiny Node.js implements a simplified version of the CommonJS module format, which is what Node.js uses. In this format:

- Each file is a module with its own scope
- Variables, functions, and classes defined in a module are private by default
- The `module.exports` or `exports` object is used to expose functionality
- The `require()` function is used to import functionality from other modules

Here's an example of a CommonJS module:

```javascript
// math.js - A module that exports math functions

// Private function (not accessible outside this module)
function validate(x, y) {
    if (typeof x !== 'number' || typeof y !== 'number') {
        throw new Error('Arguments must be numbers');
    }
}

// Public functions (exported)
exports.add = function(x, y) {
    validate(x, y);
    return x + y;
};

exports.subtract = function(x, y) {
    validate(x, y);
    return x - y;
};

// Alternative export syntax
module.exports = {
    add: (x, y) => x + y,
    subtract: (x, y) => x - y
};
```

And here's how you would use this module:

```javascript
// Import the math module
const math = require('./math');

// Use the exported functions
console.log(math.add(2, 3));      // 5
console.log(math.subtract(5, 2)); // 3
```

## The ModuleSystem Class

Let's examine how the module system is implemented in Tiny Node.js. The core of our module system is the `ModuleSystem` class, defined in `include/module_system.h`:

```cpp
class ModuleSystem {
public:
    // Constructor
    ModuleSystem(v8::Isolate* isolate);
    
    // Load a module and return its exports
    v8::Local<v8::Value> Require(const std::string& module_name);
    
    // Register a native module
    void RegisterNativeModule(const std::string& name, v8::Local<v8::Object> exports);
    
private:
    // The V8 isolate
    v8::Isolate* isolate_;
    
    // Cache of loaded modules
    std::unordered_map<std::string, v8::Persistent<v8::Value>> module_cache_;
    
    // Native modules
    std::unordered_map<std::string, v8::Persistent<v8::Object>> native_modules_;
    
    // Resolve a module path
    std::string ResolveModulePath(const std::string& module_name);
    
    // Load a JavaScript module from a file
    v8::Local<v8::Value> LoadJavaScriptModule(const std::string& filename);
    
    // Compile and execute a module
    v8::Local<v8::Value> CompileAndExecuteModule(
        const std::string& source,
        const std::string& filename);
};
```

The `ModuleSystem` class is responsible for:
- Loading modules via the `Require` method
- Caching modules to avoid redundant loading
- Registering native modules implemented in C++
- Resolving module paths
- Compiling and executing JavaScript modules

## Module Loading Process

### The `Require` Function

The `require()` function in JavaScript is implemented as a native function that calls into the `ModuleSystem::Require` method:

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

### The `Require` Method

The `ModuleSystem::Require` method handles the actual module loading:

```cpp
v8::Local<v8::Value> ModuleSystem::Require(const std::string& module_name) {
    // Create a handle scope
    v8::HandleScope handle_scope(isolate_);
    
    // Check if it's a native module
    auto native_it = native_modules_.find(module_name);
    if (native_it != native_modules_.end()) {
        // Return the native module exports
        return v8::Local<v8::Value>::New(isolate_, native_it->second);
    }
    
    // Resolve the module path
    std::string filename = ResolveModulePath(module_name);
    
    // Check if the module is already cached
    auto cache_it = module_cache_.find(filename);
    if (cache_it != module_cache_.end()) {
        // Return the cached module exports
        return v8::Local<v8::Value>::New(isolate_, cache_it->second);
    }
    
    // Load the JavaScript module
    v8::Local<v8::Value> exports = LoadJavaScriptModule(filename);
    
    // Cache the module exports
    module_cache_[filename].Reset(isolate_, exports);
    
    // Return the module exports
    return exports;
}
```

This method:
1. Checks if the requested module is a native module
2. If not, resolves the module path to a file path
3. Checks if the module is already cached
4. If not, loads the JavaScript module
5. Caches the module exports
6. Returns the module exports

### Module Path Resolution

The `ResolveModulePath` method turns a module name into a file path:

```cpp
std::string ModuleSystem::ResolveModulePath(const std::string& module_name) {
    // If the module name starts with ./ or ../, it's a relative path
    if (module_name.find("./") == 0 || module_name.find("../") == 0) {
        // Just use the module name as-is
        return module_name;
    }
    
    // For core modules (like 'fs' or 'http'), they're handled as native modules
    // For other modules, we would search in node_modules directories, but
    // that's not implemented in this simplified version
    
    // Just return the module name
    return module_name;
}
```

In a full implementation, this method would handle:
- Relative paths (starting with `.` or `..`)
- Absolute paths
- Node.js core modules
- Node.js modules in the `node_modules` directory

For simplicity, our implementation only handles relative paths and assumes other paths are handled by native modules.

### Loading JavaScript Modules

The `LoadJavaScriptModule` method loads a JavaScript module from a file:

```cpp
v8::Local<v8::Value> ModuleSystem::LoadJavaScriptModule(const std::string& filename) {
    // Read the file content
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::string error = "Module not found: " + filename;
        isolate_->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate_, error.c_str()).ToLocalChecked()));
        return v8::Local<v8::Value>();
    }
    
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Wrap the module code in a function
    std::string wrapped_source = 
        "(function(exports, require, module, __filename, __dirname) { " + 
        source + 
        "\n})";
    
    // Compile and execute the module
    return CompileAndExecuteModule(wrapped_source, filename);
}
```

This method:
1. Reads the content of the JavaScript file
2. Wraps the code in a function to provide module scope
3. Compiles and executes the module
4. Returns the module exports

### Compiling and Executing Modules

The `CompileAndExecuteModule` method compiles and executes a module:

```cpp
v8::Local<v8::Value> ModuleSystem::CompileAndExecuteModule(
    const std::string& source,
    const std::string& filename) {
    
    // Create a context for compilation
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::Context::Scope context_scope(context);
    
    // Compile the module function
    v8::Local<v8::String> source_string = 
        v8::String::NewFromUtf8(isolate_, source.c_str()).ToLocalChecked();
        
    v8::ScriptOrigin origin(
        isolate_,
        v8::String::NewFromUtf8(isolate_, filename.c_str()).ToLocalChecked()
    );
    
    v8::TryCatch try_catch(isolate_);
    v8::Local<v8::Script> script;
    
    if (!v8::Script::Compile(context, source_string, &origin).ToLocal(&script)) {
        // Report compilation errors
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Module compilation error: " << *error << std::endl;
        isolate_->ThrowException(try_catch.Exception());
        return v8::Local<v8::Value>();
    }
    
    // Execute the module function to get the wrapper function
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result) || !result->IsFunction()) {
        // Report execution errors
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Module execution error: " << *error << std::endl;
        isolate_->ThrowException(try_catch.Exception());
        return v8::Local<v8::Value>();
    }
    
    // Cast the result to a function
    v8::Local<v8::Function> module_func = v8::Local<v8::Function>::Cast(result);
    
    // Create module, exports, and require objects
    v8::Local<v8::Object> exports = v8::Object::New(isolate_);
    v8::Local<v8::Object> module = v8::Object::New(isolate_);
    
    // Set up module.exports
    module->Set(
        context,
        v8::String::NewFromUtf8(isolate_, "exports").ToLocalChecked(),
        exports
    ).Check();
    
    // Create the require function for this module
    v8::Local<v8::Function> require_func = v8::Function::New(
        context,
        RequireCallback,
        v8::External::New(isolate_, this)
    ).ToLocalChecked();
    
    // Get the directory name
    std::string dirname = filename.substr(0, filename.find_last_of('/'));
    
    // Prepare the arguments for the module function
    v8::Local<v8::Value> args[] = {
        exports,                                                               // exports
        require_func,                                                          // require
        module,                                                                // module
        v8::String::NewFromUtf8(isolate_, filename.c_str()).ToLocalChecked(),  // __filename
        v8::String::NewFromUtf8(isolate_, dirname.c_str()).ToLocalChecked()    // __dirname
    };
    
    // Call the module function
    v8::Local<v8::Value> exports_result;
    if (!module_func->Call(context, context->Global(), 5, args).ToLocal(&exports_result)) {
        // Report execution errors
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Module function execution error: " << *error << std::endl;
        isolate_->ThrowException(try_catch.Exception());
        return v8::Local<v8::Value>();
    }
    
    // Check if module.exports was modified
    v8::Local<v8::Value> maybe_exports;
    if (module->Get(context, v8::String::NewFromUtf8(isolate_, "exports").ToLocalChecked())
            .ToLocal(&maybe_exports)) {
        return maybe_exports;
    }
    
    // Default to the original exports object
    return exports;
}
```

This method:
1. Compiles the module function
2. Executes it to get the wrapper function
3. Creates the `exports`, `module`, `require`, `__filename`, and `__dirname` variables
4. Calls the wrapper function with these variables
5. Returns the module exports

## Native Modules

Native modules are modules implemented in C++ rather than JavaScript. They provide functionality that requires system access or performance optimization.

### Registering Native Modules

Native modules are registered using the `RegisterNativeModule` method:

```cpp
void ModuleSystem::RegisterNativeModule(const std::string& name, v8::Local<v8::Object> exports) {
    // Store the module exports in the native_modules_ map
    native_modules_[name].Reset(isolate_, exports);
}
```

### Example: The FS Module

The File System (FS) module is a good example of a native module. Here's a simplified version of how it's registered:

```cpp
void RegisterFsModule(v8::Isolate* isolate, ModuleSystem* module_system) {
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    
    // Create and enter a context
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    
    // Create the fs object
    v8::Local<v8::Object> fs = v8::Object::New(isolate);
    
    // Add the readFile function
    fs->Set(
        context,
        v8::String::NewFromUtf8(isolate, "readFile").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, ReadFileCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Add the writeFile function
    fs->Set(
        context,
        v8::String::NewFromUtf8(isolate, "writeFile").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, WriteFileCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Add the exists function
    fs->Set(
        context,
        v8::String::NewFromUtf8(isolate, "exists").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, ExistsCallback)->GetFunction(context).ToLocalChecked()
    ).Check();
    
    // Register the module
    module_system->RegisterNativeModule("fs", fs);
}
```

This function:
1. Creates an object to represent the FS module
2. Adds functions like `readFile`, `writeFile`, and `exists`
3. Registers the module with the module system

## Module Caching

To avoid loading the same module multiple times, the module system caches loaded modules:

```cpp
v8::Local<v8::Value> ModuleSystem::Require(const std::string& module_name) {
    // ...
    
    // Check if the module is already cached
    auto cache_it = module_cache_.find(filename);
    if (cache_it != module_cache_.end()) {
        // Return the cached module exports
        return v8::Local<v8::Value>::New(isolate_, cache_it->second);
    }
    
    // ...
    
    // Cache the module exports
    module_cache_[filename].Reset(isolate_, exports);
    
    // ...
}
```

This ensures that:
1. Each module is only loaded once
2. Modules maintain their state across multiple `require()` calls
3. Circular dependencies are handled correctly

## The Module Wrapper

When a JavaScript module is loaded, its code is wrapped in a function to provide module scope:

```javascript
(function(exports, require, module, __filename, __dirname) {
    // Module code goes here
})
```

This wrapper:
1. Provides the `exports` object for exposing functionality
2. Provides the `require` function for importing other modules
3. Provides the `module` object (which contains `exports`)
4. Provides `__filename` and `__dirname` for file and directory information
5. Creates a private scope for module-internal variables

## Differences from Node.js

Our implementation is simplified compared to Node.js:

1. **Path Resolution**: We only handle relative paths, not absolute paths or `node_modules` lookups
2. **Module Types**: We only support CommonJS modules, not ES modules
3. **Native Modules**: We only implement basic versions of `fs` and `http`
4. **Error Handling**: Our error reporting is simpler
5. **Performance**: We don't optimize for performance as much

## Extending the Module System

To add a new module type (e.g., JSON modules), you would:

1. Update the `Require` method to handle the new module type
2. Add a new method (e.g., `LoadJsonModule`) to load the new module type
3. Implement the loading logic for the new module type

For example, to add JSON module support:

```cpp
v8::Local<v8::Value> ModuleSystem::Require(const std::string& module_name) {
    // ...
    
    // Check the file extension
    if (filename.ends_with(".json")) {
        return LoadJsonModule(filename);
    } else {
        return LoadJavaScriptModule(filename);
    }
    
    // ...
}

v8::Local<v8::Value> ModuleSystem::LoadJsonModule(const std::string& filename) {
    // Read the file content
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::string error = "Module not found: " + filename;
        isolate_->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate_, error.c_str()).ToLocalChecked()));
        return v8::Local<v8::Value>();
    }
    
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Parse the JSON
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::TryCatch try_catch(isolate_);
    
    v8::Local<v8::String> json_string = 
        v8::String::NewFromUtf8(isolate_, source.c_str()).ToLocalChecked();
        
    v8::Local<v8::Value> result;
    if (!v8::JSON::Parse(context, json_string).ToLocal(&result)) {
        // Report parsing errors
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "JSON parsing error: " << *error << std::endl;
        isolate_->ThrowException(try_catch.Exception());
        return v8::Local<v8::Value>();
    }
    
    return result;
}
```

## Conclusion

The module system is a critical component of any JavaScript runtime, allowing code to be organized into reusable, maintainable units. Tiny Node.js implements a simplified version of the CommonJS module format, which provides most of the functionality needed for modular JavaScript development.

Understanding the module system is key to building and extending a JavaScript runtime, as it forms the foundation for organizing and loading code.

[← Previous: JavaScript Native Functions](05-native-functions.md) | [Next: Event Loop →](07-event-loop.md) 