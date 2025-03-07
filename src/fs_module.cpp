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
    v8::Isolate* isolate = runtime->GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Create a new context
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);
    
    // Create the fs module object
    v8::Local<v8::Object> fs = v8::Object::New(isolate);
    
    // Add the readFile function
    fs->Set(context, 
        v8::String::NewFromUtf8(isolate, "readFile").ToLocalChecked(),
        v8::Function::New(context, ReadFile, v8::External::New(isolate, runtime)).ToLocalChecked()
    ).Check();
    
    // Add the writeFile function
    fs->Set(context, 
        v8::String::NewFromUtf8(isolate, "writeFile").ToLocalChecked(),
        v8::Function::New(context, WriteFile, v8::External::New(isolate, runtime)).ToLocalChecked()
    ).Check();
    
    // Add the exists function
    fs->Set(context, 
        v8::String::NewFromUtf8(isolate, "exists").ToLocalChecked(),
        v8::Function::New(context, Exists, v8::External::New(isolate, runtime)).ToLocalChecked()
    ).Check();
    
    // Register the fs module
    runtime->GetModuleSystem()->RegisterNativeModule("fs", fs);
} 