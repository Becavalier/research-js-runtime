#include "runtime.h"
#include "event_loop.h"
#include "module.h"
#include "fs_module.h"
#include "http_module.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>

// Initialize static members
std::unique_ptr<v8::Platform> Runtime::platform_ = nullptr;

// Native setTimeout function
void SetTimeout(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 2 || !args[0]->IsFunction() || !args[1]->IsNumber()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the callback function
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[0]);
    
    // Get the delay
    uint64_t delay_ms = args[1]->IntegerValue(isolate->GetCurrentContext()).FromJust();
    
    // Get the runtime from the external data
    Runtime* runtime = static_cast<Runtime*>(args.Data().As<v8::External>()->Value());
    
    // Create a persistent handle to the callback
    v8::Global<v8::Function> persistent_callback(isolate, callback);
    
    // Schedule the task
    uint64_t task_id = runtime->ScheduleDelayedTask([persistent_callback, isolate, runtime]() {
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, v8::Context::New(isolate));
        v8::Context::Scope context_scope(context);
        
        v8::Local<v8::Function> callback = v8::Local<v8::Function>::New(isolate, persistent_callback);
        v8::Local<v8::Value> result;
        callback->Call(context, context->Global(), 0, nullptr).ToLocal(&result);
        
        // Release the persistent handle
        persistent_callback.Reset();
    }, delay_ms);
    
    // Return the task ID
    args.GetReturnValue().Set(v8::Number::New(isolate, task_id));
}

// Native clearTimeout function
void ClearTimeout(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsNumber()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the task ID
    uint64_t task_id = args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust();
    
    // Get the runtime from the external data
    Runtime* runtime = static_cast<Runtime*>(args.Data().As<v8::External>()->Value());
    
    // Cancel the task
    runtime->CancelDelayedTask(task_id);
    
    args.GetReturnValue().SetUndefined();
}

// Native print function
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    for (int i = 0; i < args.Length(); i++) {
        v8::String::Utf8Value str(isolate, args[i]);
        std::cout << *str;
        if (i < args.Length() - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    
    args.GetReturnValue().SetUndefined();
}

// Initialize the runtime
bool Runtime::Initialize() {
    // Initialize V8
    platform_ = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform_.get());
    v8::V8::Initialize();
    
    return true;
}

// Shutdown the runtime
void Runtime::Shutdown() {
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
}

// Constructor
Runtime::Runtime() : isolate_(nullptr) {
    // Create a new isolate
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate_ = v8::Isolate::New(create_params);
    
    // Create a global template
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);
    global_template_.Reset(isolate_, global);
    
    // Create the module system
    module_system_ = std::make_unique<ModuleSystem>(this);
    
    // Setup global functions
    SetupGlobalFunctions();
    
    // Register native modules
    RegisterNativeModules();
    
    // Create the event loop
    event_loop_ = std::make_unique<EventLoop>(this);
    event_loop_->Start();
}

// Destructor
Runtime::~Runtime() {
    // Stop the event loop
    if (event_loop_) {
        event_loop_->Stop();
    }
    
    // Clean up the module system
    module_system_.reset();
    
    global_template_.Reset();
    isolate_->Dispose();
}

// Read a file into a string
std::string Runtime::ReadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Create a new context
v8::Local<v8::Context> Runtime::CreateContext() {
    v8::EscapableHandleScope handle_scope(isolate_);
    v8::Local<v8::ObjectTemplate> global = v8::Local<v8::ObjectTemplate>::New(isolate_, global_template_);
    return handle_scope.Escape(v8::Context::New(isolate_, nullptr, global));
}

// Execute a JavaScript file
bool Runtime::ExecuteFile(const std::string& filename) {
    std::string source = ReadFile(filename);
    if (source.empty()) {
        return false;
    }
    
    return ExecuteString(source, filename);
}

// Execute a JavaScript string
bool Runtime::ExecuteString(const std::string& source, const std::string& source_name) {
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    
    // Create a context
    v8::Local<v8::Context> context = CreateContext();
    v8::Context::Scope context_scope(context);
    
    // Create a string containing the JavaScript source code
    v8::Local<v8::String> source_str = v8::String::NewFromUtf8(
        isolate_, source.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    
    v8::Local<v8::String> name_str;
    if (!source_name.empty()) {
        name_str = v8::String::NewFromUtf8(
            isolate_, source_name.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    } else {
        name_str = v8::String::NewFromUtf8(
            isolate_, "<string>", v8::NewStringType::kNormal).ToLocalChecked();
    }
    
    // Compile the source code
    v8::TryCatch try_catch(isolate_);
    v8::ScriptOrigin origin(name_str);
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source_str, &origin).ToLocal(&script)) {
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Compilation error: " << *error << std::endl;
        return false;
    }
    
    // Run the script
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
        v8::String::Utf8Value error(isolate_, try_catch.Exception());
        std::cerr << "Execution error: " << *error << std::endl;
        return false;
    }
    
    // Convert the result to a string and print it
    if (!result->IsUndefined()) {
        v8::String::Utf8Value utf8(isolate_, result);
        std::cout << *utf8 << std::endl;
    }
    
    return true;
}

// Register a native function
void Runtime::RegisterNativeFunction(const std::string& name, v8::FunctionCallback callback) {
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    
    v8::Local<v8::ObjectTemplate> global = v8::Local<v8::ObjectTemplate>::New(isolate_, global_template_);
    global->Set(
        v8::String::NewFromUtf8(isolate_, name.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
        v8::FunctionTemplate::New(isolate_, callback, v8::External::New(isolate_, this))
    );
    
    global_template_.Reset(isolate_, global);
}

// Get the event loop
EventLoop* Runtime::GetEventLoop() const {
    return event_loop_.get();
}

// Get the module system
ModuleSystem* Runtime::GetModuleSystem() const {
    return module_system_.get();
}

// Get the isolate
v8::Isolate* Runtime::GetIsolate() const {
    return isolate_;
}

// Schedule a task on the event loop
void Runtime::ScheduleTask(std::function<void()> task) {
    if (event_loop_) {
        event_loop_->ScheduleTask(task);
    }
}

// Schedule a delayed task on the event loop
uint64_t Runtime::ScheduleDelayedTask(std::function<void()> task, uint64_t delay_ms) {
    if (event_loop_) {
        return event_loop_->ScheduleDelayedTask(task, delay_ms);
    }
    return 0;
}

// Cancel a delayed task
void Runtime::CancelDelayedTask(uint64_t task_id) {
    if (event_loop_) {
        event_loop_->CancelDelayedTask(task_id);
    }
}

// Setup global functions
void Runtime::SetupGlobalFunctions() {
    // Register the print function
    RegisterNativeFunction("print", Print);
    
    // Register the setTimeout function
    RegisterNativeFunction("setTimeout", SetTimeout);
    
    // Register the clearTimeout function
    RegisterNativeFunction("clearTimeout", ClearTimeout);
    
    // Register the require function
    RegisterNativeFunction("require", Require);
}

// Register native modules
void Runtime::RegisterNativeModules() {
    // Register the fs module
    RegisterFsModule(this);
    
    // Register the http module
    RegisterHttpModule(this);
} 