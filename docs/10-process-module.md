# 10. Process Module

The process module is a core part of any JavaScript runtime, providing access to the current process, system information, and environment variables. This chapter explores how Tiny Node.js implements its process module, which bridges the gap between JavaScript and the underlying operating system.

## The Role of the Process Module

The process module provides JavaScript code with access to:
- Command-line arguments
- Environment variables
- System information (platform, architecture)
- Process control (exit, signals)
- Current working directory

This functionality is essential for creating robust applications that can interact with the operating system, read configuration from the environment, and properly handle process lifecycle.

## Process Module Architecture

Unlike other modules in Tiny Node.js, the process module is implemented as a global object rather than a module that needs to be explicitly required. This mirrors Node.js's approach, where `process` is globally available in all scripts.

The process module in Tiny Node.js consists of several components:

1. **Global Registration**: The process object is added to the global scope
2. **Command-line Arguments**: Access to the arguments passed to the script
3. **Environment Variables**: Access to the system's environment variables
4. **System Information**: Access to platform, architecture, and version information
5. **Process Control**: Methods to exit the process and manipulate its state
6. **Working Directory**: Access to and manipulation of the current working directory

Let's examine each of these components.

## Global Registration

The process module is registered as a global object when each context is created:

```cpp
void RegisterProcessModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    std::cout << "RegisterProcessModule: Starting..." << std::endl;
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    std::cout << "RegisterProcessModule: Got isolate" << std::endl;
    
    // Create the process object
    v8::Local<v8::Object> process = v8::Object::New(isolate);
    std::cout << "RegisterProcessModule: Created process object" << std::endl;
    
    // Add the argv property
    std::cout << "RegisterProcessModule: Adding argv..." << std::endl;
    AddArgvToProcess(isolate, context, process);
    
    // Add the env property
    std::cout << "RegisterProcessModule: Adding env..." << std::endl;
    AddEnvToProcess(isolate, context, process);
    
    // Add version information
    std::cout << "RegisterProcessModule: Adding version info..." << std::endl;
    AddVersionToProcess(isolate, context, process);
    
    // Add platform information
    std::cout << "RegisterProcessModule: Adding platform info..." << std::endl;
    AddPlatformToProcess(isolate, context, process);
    
    // Add the exit function
    std::cout << "RegisterProcessModule: Adding exit function..." << std::endl;
    AddExitToProcess(isolate, context, process);
    
    // Add the cwd function
    std::cout << "RegisterProcessModule: Adding cwd function..." << std::endl;
    AddCwdToProcess(isolate, context, process);
    
    // Add the process object to the global object
    std::cout << "RegisterProcessModule: Registering as global object..." << std::endl;
    context->Global()->Set(
        context,
        v8::String::NewFromUtf8(isolate, "process").ToLocalChecked(),
        process
    ).Check();
    
    std::cout << "RegisterProcessModule: Complete" << std::endl;
}
```

This function:
1. Creates a new JavaScript object to represent the process
2. Adds properties and methods to this object
3. Sets the object as a property of the global object with the name "process"

Unlike other modules, this function is called during context creation in the `Runtime::CreateContext` method, making the process object available in every JavaScript context.

## Command-line Arguments

The `argv` property provides access to the command-line arguments:

```cpp
void AddArgvToProcess(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> process) {
    // Get the arguments from the runtime
    Runtime* runtime = static_cast<Runtime*>(isolate->GetData(RUNTIME_INDEX));
    const std::vector<std::string>& args = runtime->GetArgs();
    
    // Create the argv array
    v8::Local<v8::Array> argv = v8::Array::New(isolate, args.size());
    
    // Add each argument to the array
    for (size_t i = 0; i < args.size(); i++) {
        argv->Set(
            context,
            i,
            v8::String::NewFromUtf8(isolate, args[i].c_str()).ToLocalChecked()
        ).Check();
    }
    
    // Add the argv array to the process object
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "argv").ToLocalChecked(),
        argv
    ).Check();
}
```

This function:
1. Gets the command-line arguments from the runtime
2. Creates a JavaScript array to hold the arguments
3. Adds each argument to the array
4. Sets the array as the `argv` property of the process object

The arguments are stored in the `Runtime` class, which receives them from the `main` function.

## Environment Variables

The `env` property provides access to environment variables:

```cpp
void AddEnvToProcess(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> process) {
    // Create the env object
    v8::Local<v8::Object> env = v8::Object::New(isolate);
    
    // Add environment variables to the env object
    #ifdef _WIN32
    // Windows implementation
    wchar_t* env_block = GetEnvironmentStringsW();
    if (env_block) {
        for (wchar_t* env_str = env_block; *env_str; env_str += wcslen(env_str) + 1) {
            std::wstring env_wstr(env_str);
            std::string env_string(env_wstr.begin(), env_wstr.end());
            size_t pos = env_string.find('=');
            if (pos != std::string::npos) {
                std::string key = env_string.substr(0, pos);
                std::string value = env_string.substr(pos + 1);
                env->Set(
                    context,
                    v8::String::NewFromUtf8(isolate, key.c_str()).ToLocalChecked(),
                    v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked()
                ).Check();
            }
        }
        FreeEnvironmentStringsW(env_block);
    }
    #else
    // POSIX implementation
    extern char** environ;
    for (char** env_ptr = environ; *env_ptr; env_ptr++) {
        std::string env_string(*env_ptr);
        size_t pos = env_string.find('=');
        if (pos != std::string::npos) {
            std::string key = env_string.substr(0, pos);
            std::string value = env_string.substr(pos + 1);
            env->Set(
                context,
                v8::String::NewFromUtf8(isolate, key.c_str()).ToLocalChecked(),
                v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked()
            ).Check();
        }
    }
    #endif
    
    // Add the env object to the process object
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "env").ToLocalChecked(),
        env
    ).Check();
}
```

This function:
1. Creates a JavaScript object to hold the environment variables
2. Gets the environment variables from the system
3. Adds each environment variable as a property of the object
4. Sets the object as the `env` property of the process object

The implementation is different for Windows and POSIX systems due to differences in how environment variables are accessed.

## System Information

The process module includes information about the system, such as the version of the runtime, the platform, and the architecture:

```cpp
void AddVersionToProcess(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> process) {
    // Add the version string
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "version").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, "1.0.0").ToLocalChecked()
    ).Check();
    
    // Create the versions object
    v8::Local<v8::Object> versions = v8::Object::New(isolate);
    
    // Add V8 version
    versions->Set(
        context,
        v8::String::NewFromUtf8(isolate, "v8").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, v8::V8::GetVersion()).ToLocalChecked()
    ).Check();
    
    // Add node version
    versions->Set(
        context,
        v8::String::NewFromUtf8(isolate, "node").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, "1.0.0").ToLocalChecked()
    ).Check();
    
    // Add the versions object to the process object
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "versions").ToLocalChecked(),
        versions
    ).Check();
}

void AddPlatformToProcess(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> process) {
    // Determine the platform
    std::string platform;
    #ifdef _WIN32
    platform = "win32";
    #elif __APPLE__
    platform = "darwin";
    #elif __linux__
    platform = "linux";
    #else
    platform = "unknown";
    #endif
    
    // Add the platform string
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "platform").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, platform.c_str()).ToLocalChecked()
    ).Check();
    
    // Determine the architecture
    std::string arch;
    #if defined(__x86_64__) || defined(_M_X64)
    arch = "x64";
    #elif defined(__i386__) || defined(_M_IX86)
    arch = "ia32";
    #elif defined(__arm__) || defined(_M_ARM)
    arch = "arm";
    #elif defined(__aarch64__)
    arch = "arm64";
    #else
    arch = "unknown";
    #endif
    
    // Add the architecture string
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "arch").ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, arch.c_str()).ToLocalChecked()
    ).Check();
}
```

These functions:
1. Add version information, including the version of the runtime and V8
2. Determine and add the platform (e.g., "win32", "darwin", "linux")
3. Determine and add the architecture (e.g., "x64", "ia32", "arm64")

This information is useful for scripts that need to behave differently depending on the platform or architecture.

## Process Control

The `exit` method allows scripts to exit the process with a specified exit code:

```cpp
void AddExitToProcess(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> process) {
    // Add the exit function
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "exit").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            // Get the isolate
            v8::Isolate* isolate = args.GetIsolate();
            
            // Get the exit code
            int exit_code = 0;
            if (args.Length() > 0 && args[0]->IsNumber()) {
                exit_code = args[0]->Int32Value(isolate->GetCurrentContext()).FromJust();
            }
            
            // Log the exit
            std::cout << "Exiting with code " << exit_code << std::endl;
            
            // Exit the process
            exit(exit_code);
        })->GetFunction(context).ToLocalChecked()
    ).Check();
}
```

This function adds an `exit` method to the process object that:
1. Gets the exit code from the arguments (defaulting to 0)
2. Logs the exit
3. Calls the C `exit` function to terminate the process

This allows scripts to terminate the process cleanly, with a specified exit code.

## Working Directory

The `cwd` method provides access to the current working directory:

```cpp
void AddCwdToProcess(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> process) {
    // Add the cwd function
    process->Set(
        context,
        v8::String::NewFromUtf8(isolate, "cwd").ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            // Get the isolate
            v8::Isolate* isolate = args.GetIsolate();
            
            // Get the current working directory
            #ifdef _WIN32
            // Windows implementation
            wchar_t buffer[MAX_PATH];
            if (GetCurrentDirectoryW(MAX_PATH, buffer)) {
                std::wstring cwd_wstr(buffer);
                std::string cwd_str(cwd_wstr.begin(), cwd_wstr.end());
                args.GetReturnValue().Set(
                    v8::String::NewFromUtf8(isolate, cwd_str.c_str()).ToLocalChecked()
                );
            } else {
                args.GetReturnValue().Set(
                    v8::String::NewFromUtf8(isolate, "").ToLocalChecked()
                );
            }
            #else
            // POSIX implementation
            char buffer[PATH_MAX];
            if (getcwd(buffer, PATH_MAX)) {
                args.GetReturnValue().Set(
                    v8::String::NewFromUtf8(isolate, buffer).ToLocalChecked()
                );
            } else {
                args.GetReturnValue().Set(
                    v8::String::NewFromUtf8(isolate, "").ToLocalChecked()
                );
            }
            #endif
        })->GetFunction(context).ToLocalChecked()
    ).Check();
}
```

This function adds a `cwd` method to the process object that:
1. Gets the current working directory from the system
2. Returns it as a string

This allows scripts to determine the directory they're running in, which is useful for resolving relative paths.

## Using the Process Module in JavaScript

Once the process module is implemented and registered, it can be used in JavaScript code like this:

```javascript
// The process object is globally available, no need to require it

// Access command-line arguments
console.log('Command line arguments:');
process.argv.forEach((arg, index) => {
    console.log(`  argv[${index}] = ${arg}`);
});

// Access environment variables
console.log('\nEnvironment variables:');
console.log(`  HOME = ${process.env.HOME}`);
console.log(`  PATH = ${process.env.PATH}`);
console.log(`  USER = ${process.env.USER}`);

// Access system information
console.log('\nSystem information:');
console.log(`  Platform: ${process.platform}`);
console.log(`  Architecture: ${process.arch}`);
console.log(`  Node.js version: ${process.version}`);
console.log(`  V8 version: ${process.versions.v8}`);

// Get the current working directory
console.log('\nCurrent working directory:');
console.log(`  ${process.cwd()}`);

// Exit the process (uncomment to use)
// process.exit(0);
```

This example demonstrates how to:
1. Access command-line arguments through `process.argv`
2. Access environment variables through `process.env`
3. Access system information through `process.platform`, `process.arch`, etc.
4. Get the current working directory through `process.cwd()`
5. Exit the process through `process.exit()`

## Extending the Process Module

The process module can be extended with additional functionality:

1. **Signal Handling**: Adding methods for handling signals like SIGINT, SIGTERM
2. **Standard Streams**: Adding properties for stdin, stdout, stderr
3. **Memory Usage**: Adding methods for getting memory usage information
4. **CPU Usage**: Adding methods for getting CPU usage information
5. **Child Process Management**: Adding methods for spawning and managing child processes

Each of these can be implemented following the same pattern as the existing methods, providing access to additional system functionality.

## Security Considerations

The process module provides access to sensitive system information and functionality. Here are some security considerations:

1. **Environment Variables**: May contain sensitive information like API keys and passwords
2. **Command-line Arguments**: May contain sensitive information
3. **Process Control**: The `exit` method can terminate the process, potentially causing data loss

In a production environment, you might want to restrict access to certain parts of the process module or sanitize sensitive information.

## Performance Considerations

Most process module functionality is accessed infrequently, so performance is generally not a concern. However, some properties like `process.env` are accessed frequently, so it might be worth optimizing their implementation.

## Cross-Platform Considerations

The process module needs to work on different platforms, which requires platform-specific code. Our implementation includes separate code paths for Windows and POSIX systems, but a more complete implementation would handle more platforms and edge cases.

## Conclusion

The process module is a fundamental component of any JavaScript runtime, providing access to the system and the runtime environment. By implementing the process module as a global object, Tiny Node.js provides a convenient way for scripts to interact with the system, access configuration, and control the process lifecycle.

Understanding the implementation of the process module provides insights into how JavaScript runtimes bridge the gap between JavaScript code and the underlying operating system, and how global objects are implemented in V8.

[← Previous: HTTP Server Implementation](09-http-module.md) | [Next: Testing and Debugging →](11-testing-debugging.md) 