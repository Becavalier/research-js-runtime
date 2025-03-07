#include "module.h"
#include "runtime.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

// Module constructor
Module::Module(Runtime* runtime, const std::string& id, const std::string& filename)
    : runtime_(runtime), id_(id), filename_(filename), loaded_(false) {
}

// Module destructor
Module::~Module() {
    exports_.Reset();
}

// Load the module
bool Module::Load() {
    if (loaded_) {
        return true;
    }
    
    // Check if the file exists
    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Failed to open module file: " << filename_ << std::endl;
        return false;
    }
    
    // Read the file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // Wrap the source in a function to create a module scope
    std::string wrapped_source = "(function(exports, require, module, __filename, __dirname) {\n";
    wrapped_source += source;
    wrapped_source += "\n})";
    
    // Get the isolate
    v8::Isolate* isolate = runtime_->GetIsolate();
    
    // Create a handle scope
    v8::HandleScope handle_scope(isolate);
    
    // Create a context
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);
    
    // Compile the wrapped source
    v8::Local<v8::String> source_str = v8::String::NewFromUtf8(
        isolate, wrapped_source.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source_str).ToLocal(&script)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cerr << "Failed to compile module: " << id_ << " - " << *error << std::endl;
        return false;
    }
    
    // Run the script to get the module function
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cerr << "Failed to run module: " << id_ << " - " << *error << std::endl;
        return false;
    }
    
    // Cast the result to a function
    v8::Local<v8::Function> module_func = v8::Local<v8::Function>::Cast(result);
    
    // Create the exports object
    v8::Local<v8::Object> exports = v8::Object::New(isolate);
    
    // Create the module object
    v8::Local<v8::Object> module = v8::Object::New(isolate);
    module->Set(context, v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked(), exports).Check();
    
    // Get the directory name
    std::filesystem::path path(filename_);
    std::string dirname = path.parent_path().string();
    
    // Create the arguments for the module function
    v8::Local<v8::Value> args[5] = {
        exports,
        v8::Function::New(context, Require, v8::External::New(isolate, runtime_)).ToLocalChecked(),
        module,
        v8::String::NewFromUtf8(isolate, filename_.c_str()).ToLocalChecked(),
        v8::String::NewFromUtf8(isolate, dirname.c_str()).ToLocalChecked()
    };
    
    // Call the module function
    if (module_func->Call(context, context->Global(), 5, args).IsEmpty()) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        std::cerr << "Failed to execute module: " << id_ << " - " << *error << std::endl;
        return false;
    }
    
    // Get the exports from the module object
    v8::Local<v8::Value> exports_value;
    if (!module->Get(context, v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked()).ToLocal(&exports_value)) {
        std::cerr << "Failed to get exports from module: " << id_ << std::endl;
        return false;
    }
    
    // Store the exports
    exports_.Reset(isolate, exports_value.As<v8::Object>());
    
    // Mark the module as loaded
    loaded_ = true;
    
    return true;
}

// Get the module ID
const std::string& Module::GetId() const {
    return id_;
}

// Get the module filename
const std::string& Module::GetFilename() const {
    return filename_;
}

// Get the module exports
v8::Local<v8::Object> Module::GetExports(v8::Isolate* isolate) {
    return v8::Local<v8::Object>::New(isolate, exports_);
}

// ModuleSystem constructor
ModuleSystem::ModuleSystem(Runtime* runtime)
    : runtime_(runtime) {
}

// ModuleSystem destructor
ModuleSystem::~ModuleSystem() {
    modules_.clear();
    
    for (auto& pair : native_modules_) {
        pair.second.Reset();
    }
    native_modules_.clear();
}

// Require a module
v8::Local<v8::Object> ModuleSystem::Require(const std::string& module_id) {
    v8::Isolate* isolate = runtime_->GetIsolate();
    
    // Check if it's a native module
    auto native_it = native_modules_.find(module_id);
    if (native_it != native_modules_.end()) {
        return v8::Local<v8::Object>::New(isolate, native_it->second);
    }
    
    // Check if the module is already loaded
    auto it = modules_.find(module_id);
    if (it != modules_.end()) {
        return it->second->GetExports(isolate);
    }
    
    // Resolve the module ID to a filename
    std::string filename = ResolveModuleId(module_id);
    if (filename.empty()) {
        // Module not found
        v8::Local<v8::Object> empty = v8::Object::New(isolate);
        return empty;
    }
    
    // Create a new module
    std::shared_ptr<Module> module = std::make_shared<Module>(runtime_, module_id, filename);
    
    // Load the module
    if (!module->Load()) {
        // Failed to load the module
        v8::Local<v8::Object> empty = v8::Object::New(isolate);
        return empty;
    }
    
    // Store the module
    modules_[module_id] = module;
    
    // Return the module exports
    return module->GetExports(isolate);
}

// Register a native module
void ModuleSystem::RegisterNativeModule(const std::string& module_id, v8::Local<v8::Object> exports) {
    v8::Isolate* isolate = runtime_->GetIsolate();
    native_modules_[module_id].Reset(isolate, exports);
}

// Get the runtime
Runtime* ModuleSystem::GetRuntime() const {
    return runtime_;
}

// Resolve a module ID to a filename
std::string ModuleSystem::ResolveModuleId(const std::string& module_id) {
    // For simplicity, we'll just append .js to the module ID
    // In a real implementation, we would have a more sophisticated resolution algorithm
    return module_id + ".js";
}

// Native require function
void Require(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the module ID
    v8::String::Utf8Value module_id(isolate, args[0]);
    
    // Get the runtime from the external data
    Runtime* runtime = static_cast<Runtime*>(args.Data().As<v8::External>()->Value());
    
    // Get the module system
    ModuleSystem* module_system = runtime->GetModuleSystem();
    
    // Require the module
    v8::Local<v8::Object> exports = module_system->Require(*module_id);
    
    // Return the exports
    args.GetReturnValue().Set(exports);
} 