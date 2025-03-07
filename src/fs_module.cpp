#include "runtime.h"
#include "module.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

// Native readFile function
void ReadFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the filename
    v8::String::Utf8Value filename(isolate, args[0]);
    
    // Open the file
    std::ifstream file(*filename);
    if (!file.is_open()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Failed to open file").ToLocalChecked()));
        return;
    }
    
    // Read the file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Return the file content
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, content.c_str()).ToLocalChecked());
}

// Native writeFile function
void WriteFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the filename
    v8::String::Utf8Value filename(isolate, args[0]);
    
    // Get the content
    v8::String::Utf8Value content(isolate, args[1]);
    
    // Open the file
    std::ofstream file(*filename);
    if (!file.is_open()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Failed to open file").ToLocalChecked()));
        return;
    }
    
    // Write the content
    file << *content;
    
    // Return true if successful
    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

// Native exists function
void Exists(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the path
    v8::String::Utf8Value path(isolate, args[0]);
    
    // Check if the path exists
    bool exists = std::filesystem::exists(*path);
    
    // Return the result
    args.GetReturnValue().Set(v8::Boolean::New(isolate, exists));
}

// Register the fs module
void RegisterFsModule(Runtime* runtime) {
    std::cout << "RegisterFsModule: Starting..." << std::endl;
    
    try {
        v8::Isolate* isolate = runtime->GetIsolate();
        std::cout << "RegisterFsModule: Got isolate" << std::endl;
        
        // Create a handle scope
        v8::HandleScope scope(isolate);
        std::cout << "RegisterFsModule: Created handle scope" << std::endl;
        
        // Create a new context for module initialization
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);
        std::cout << "RegisterFsModule: Created and entered context" << std::endl;
        
        // Create the fs module object
        v8::Local<v8::Object> fs = v8::Object::New(isolate);
        std::cout << "RegisterFsModule: Created fs object" << std::endl;
        
        // Add the readFile function
        std::cout << "RegisterFsModule: Adding readFile function..." << std::endl;
        v8::Local<v8::Function> readFile = v8::Function::New(
            context, 
            ReadFile
        ).ToLocalChecked();
        fs->Set(
            context,
            v8::String::NewFromUtf8(isolate, "readFile").ToLocalChecked(),
            readFile
        ).Check();
        
        // Add the writeFile function
        std::cout << "RegisterFsModule: Adding writeFile function..." << std::endl;
        v8::Local<v8::Function> writeFile = v8::Function::New(
            context, 
            WriteFile
        ).ToLocalChecked();
        fs->Set(
            context,
            v8::String::NewFromUtf8(isolate, "writeFile").ToLocalChecked(),
            writeFile
        ).Check();
        
        // Add the exists function
        std::cout << "RegisterFsModule: Adding exists function..." << std::endl;
        v8::Local<v8::Function> exists = v8::Function::New(
            context, 
            Exists
        ).ToLocalChecked();
        fs->Set(
            context,
            v8::String::NewFromUtf8(isolate, "exists").ToLocalChecked(),
            exists
        ).Check();
        
        // Register the fs module
        std::cout << "RegisterFsModule: Registering module with ModuleSystem..." << std::endl;
        runtime->GetModuleSystem()->RegisterNativeModule("fs", fs);
        
        std::cout << "RegisterFsModule: Complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in RegisterFsModule: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in RegisterFsModule" << std::endl;
    }
} 