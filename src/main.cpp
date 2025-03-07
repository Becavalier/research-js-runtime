#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "runtime.h"
#include "process_module.h"

/**
 * @brief Native print function exposed to JavaScript
 * 
 * This function is registered as a global function in the JavaScript environment.
 * It allows JavaScript code to print to the console, similar to console.log in Node.js.
 * 
 * @param args The JavaScript function arguments
 */
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Get the current isolate (V8's execution environment)
    v8::Isolate* isolate = args.GetIsolate();
    
    // Create a handle scope to manage the local handles
    v8::HandleScope scope(isolate);
    
    // Iterate through all arguments and print them
    for (int i = 0; i < args.Length(); i++) {
        // Convert the JavaScript value to a C++ string
        v8::String::Utf8Value str(isolate, args[i]);
        std::cout << *str;
        
        // Add a space between arguments (but not after the last one)
        if (i < args.Length() - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    
    // Return undefined (like most Node.js functions)
    args.GetReturnValue().SetUndefined();
}

/**
 * @brief Main entry point for the tiny Node.js runtime
 * 
 * This function initializes the JavaScript runtime, registers native modules,
 * executes the provided JavaScript file, and keeps the event loop running.
 * 
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return int Exit code (0 for success, non-zero for failure)
 */
int main(int argc, char* argv[]) {
    std::cout << "Starting main function..." << std::endl;
    
    // Check if a JavaScript file was provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <script.js>" << std::endl;
        return 1;
    }
    
    std::cout << "Initializing runtime..." << std::endl;
    
    // Initialize the V8 platform
    if (!Runtime::Initialize()) {
        std::cerr << "Failed to initialize V8" << std::endl;
        return 1;
    }
    
    std::cout << "Creating runtime instance..." << std::endl;
    
    // Create a new runtime instance
    Runtime runtime;
    
    // Register the native print function
    std::cout << "Registering print function..." << std::endl;
    runtime.RegisterNativeFunction("print", Print);
    
    // Register the process module
    std::cout << "Registering process module..." << std::endl;
    RegisterProcessModule(&runtime, argc, argv);
    
    std::cout << "Executing file: " << argv[1] << std::endl;
    
    // Execute the JavaScript file
    if (!runtime.ExecuteFile(argv[1])) {
        std::cerr << "Failed to execute file: " << argv[1] << std::endl;
        Runtime::Shutdown();
        return 1;
    }
    
    std::cout << "File executed successfully, waiting for event loop..." << std::endl;
    
    // Wait for a moment to allow async operations to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "Shutting down runtime..." << std::endl;
    
    // Shutdown the V8 platform
    Runtime::Shutdown();
    
    std::cout << "Runtime shutdown complete" << std::endl;
    
    return 0;
} 