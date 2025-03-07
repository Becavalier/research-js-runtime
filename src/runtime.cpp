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
    Runtime* runtime = static_cast<Runtime*>(isolate->GetData(0));
    
    // Create a persistent handle to the callback
    v8::Global<v8::Function>* persistent_callback = new v8::Global<v8::Function>(isolate, callback);
    
    // Store the current context in a persistent handle
    v8::Global<v8::Context>* persistent_context = new v8::Global<v8::Context>(isolate, isolate->GetCurrentContext());
    
    // Schedule the task
    uint64_t task_id = runtime->ScheduleDelayedTask([isolate, persistent_context, persistent_callback]() {
        v8::HandleScope handle_scope(isolate);
        
        // Use the stored context
        v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, *persistent_context);
        v8::Context::Scope context_scope(context);
        
        v8::Local<v8::Function> callback = v8::Local<v8::Function>::New(isolate, *persistent_callback);
        v8::Local<v8::Value> result;
        v8::MaybeLocal<v8::Value> maybe_result = callback->Call(context, context->Global(), 0, nullptr);
        if (!maybe_result.IsEmpty()) {
            bool success = maybe_result.ToLocal(&result);
            if (!success) {
                std::cerr << "Failed to convert MaybeLocal to Local in SetTimeout callback" << std::endl;
            }
        }
        
        // Release the persistent handles
        persistent_callback->Reset();
        delete persistent_callback;
        persistent_context->Reset();
        delete persistent_context;
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
    
    // Get the runtime from the isolate's data slot
    Runtime* runtime = static_cast<Runtime*>(isolate->GetData(0));
    
    // Cancel the task
    runtime->CancelDelayedTask(task_id);
    
    args.GetReturnValue().SetUndefined();
}

// Native print function
static void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
    std::cout << "Initializing V8..." << std::endl;
    
    // Initialize V8 with the correct parameters for the custom build
    platform_ = v8::platform::NewDefaultPlatform(
        0,  // thread_pool_size (0 means default)
        v8::platform::IdleTaskSupport::kDisabled,
        v8::platform::InProcessStackDumping::kDisabled,
        nullptr  // tracing_controller
    );
    
    std::cout << "Platform created, initializing V8 platform..." << std::endl;
    v8::V8::InitializePlatform(platform_.get());
    
    std::cout << "V8 platform initialized, initializing V8..." << std::endl;
    v8::V8::Initialize();
    
    std::cout << "V8 initialized successfully" << std::endl;
    return true;
}

// Shutdown the runtime
void Runtime::Shutdown() {
    std::cout << "Shutdown: Starting..." << std::endl;
    
    try {
        // Skip V8 disposal for now
        std::cout << "Shutdown: Skipping V8 disposal" << std::endl;
        // v8::V8::Dispose();
        
        // Skip platform reset for now
        std::cout << "Shutdown: Skipping platform reset" << std::endl;
        // platform_.reset();
        
        std::cout << "Shutdown: Complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in Shutdown: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in Shutdown" << std::endl;
    }
}

// Constructor
Runtime::Runtime() : isolate_(nullptr) {
    std::cout << "Runtime constructor: Creating isolate..." << std::endl;
    
    // Create the isolate
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate_ = v8::Isolate::New(create_params);
    
    std::cout << "Runtime constructor: Isolate created" << std::endl;
    
    // Store this runtime instance in the isolate's data slot
    isolate_->SetData(0, this);
    
    // Create a handle scope
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    
    std::cout << "Runtime constructor: Creating global template..." << std::endl;
    
    // Create a global object template
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);
    
    // Store the global template
    global_template_.Reset(isolate_, global);
    
    std::cout << "Runtime constructor: Creating module system..." << std::endl;
    
    // Create the module system
    module_system_ = std::make_unique<ModuleSystem>(this);
    
    std::cout << "Runtime constructor: Setting up global functions..." << std::endl;
    
    // Setup global functions
    SetupGlobalFunctions();
    
    std::cout << "Runtime constructor: Registering native modules..." << std::endl;
    
    // Register native modules
    RegisterNativeModules();
    
    // Temporarily disable event loop creation
    std::cout << "Runtime constructor: Creating event loop..." << std::endl;
    event_loop_ = std::make_unique<EventLoop>(this);
    event_loop_->Start();
    
    std::cout << "Runtime constructor: Complete" << std::endl;
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
    std::cout << "CreateContext: Starting..." << std::endl;
    
    try {
        v8::EscapableHandleScope handle_scope(isolate_);
        std::cout << "CreateContext: Created escapable handle scope" << std::endl;
        
        // Create a new context directly without using the global template
        v8::Local<v8::Context> context = v8::Context::New(isolate_);
        std::cout << "CreateContext: Created context" << std::endl;
        
        // Enter the context
        v8::Context::Scope context_scope(context);
        std::cout << "CreateContext: Entered context" << std::endl;
        
        // Add the native functions to the context
        std::cout << "CreateContext: Adding native functions to context..." << std::endl;
        v8::Local<v8::Object> global = context->Global();
        
        for (const auto& [name, callback] : native_functions_) {
            std::cout << "CreateContext: Adding function " << name << " to context" << std::endl;
            
            v8::Local<v8::String> function_name = v8::String::NewFromUtf8(
                isolate_, name.c_str()).ToLocalChecked();
            
            v8::Local<v8::Function> function = v8::Function::New(
                context, callback).ToLocalChecked();
            
            global->Set(context, function_name, function).Check();
        }
        
        // Add native modules to the global object for direct access
        // This makes 'process' available as a global like in Node.js
        v8::Local<v8::Value> process_value;
        if (module_system_->GetNativeModule("process", &process_value)) {
            std::cout << "CreateContext: Adding process to global object" << std::endl;
            global->Set(
                context, 
                v8::String::NewFromUtf8(isolate_, "process").ToLocalChecked(),
                process_value
            ).Check();
        }
        
        std::cout << "CreateContext: Complete" << std::endl;
        
        return handle_scope.Escape(context);
    } catch (const std::exception& e) {
        std::cerr << "Exception in CreateContext: " << e.what() << std::endl;
        return v8::Local<v8::Context>();
    } catch (...) {
        std::cerr << "Unknown exception in CreateContext" << std::endl;
        return v8::Local<v8::Context>();
    }
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
    std::cout << "ExecuteString: Starting..." << std::endl;
    
    try {
        v8::Isolate::Scope isolate_scope(isolate_);
        std::cout << "ExecuteString: Created isolate scope" << std::endl;
        
        v8::HandleScope handle_scope(isolate_);
        std::cout << "ExecuteString: Created handle scope" << std::endl;
        
        // Create a context
        v8::Local<v8::Context> context = CreateContext();
        std::cout << "ExecuteString: Created context" << std::endl;
        
        v8::Context::Scope context_scope(context);
        std::cout << "ExecuteString: Created context scope" << std::endl;
        
        // Create a string containing the JavaScript source code
        v8::Local<v8::String> source_str = v8::String::NewFromUtf8(
            isolate_, source.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
        std::cout << "ExecuteString: Created source string" << std::endl;
        
        v8::Local<v8::String> name_str;
        if (!source_name.empty()) {
            name_str = v8::String::NewFromUtf8(
                isolate_, source_name.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
        } else {
            name_str = v8::String::NewFromUtf8(
                isolate_, "<string>", v8::NewStringType::kNormal).ToLocalChecked();
        }
        std::cout << "ExecuteString: Created name string" << std::endl;
        
        // Compile the source code
        v8::TryCatch try_catch(isolate_);
        std::cout << "ExecuteString: Created try_catch" << std::endl;
        
        v8::ScriptOrigin origin(name_str);
        std::cout << "ExecuteString: Created script origin" << std::endl;
        
        // Compile the script
        std::cout << "ExecuteString: Compiling script..." << std::endl;
        v8::Local<v8::Script> script;
        if (!v8::Script::Compile(context, source_str, &origin).ToLocal(&script)) {
            v8::String::Utf8Value error(isolate_, try_catch.Exception());
            std::cerr << "Compilation error: " << *error << std::endl;
            return false;
        }
        std::cout << "ExecuteString: Script compiled successfully" << std::endl;
        
        // Run the script
        std::cout << "ExecuteString: Running script..." << std::endl;
        v8::Local<v8::Value> result;
        if (!script->Run(context).ToLocal(&result)) {
            v8::String::Utf8Value error(isolate_, try_catch.Exception());
            std::cerr << "Execution error: " << *error << std::endl;
            return false;
        }
        std::cout << "ExecuteString: Script executed successfully" << std::endl;
        
        // Convert the result to a string and print it
        if (!result->IsUndefined()) {
            v8::String::Utf8Value utf8(isolate_, result);
            std::cout << "Script result: " << *utf8 << std::endl;
        }
        
        std::cout << "ExecuteString: Complete" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in ExecuteString: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown exception in ExecuteString" << std::endl;
        return false;
    }
}

// Register a native function
void Runtime::RegisterNativeFunction(const std::string& name, v8::FunctionCallback callback) {
    std::cout << "RegisterNativeFunction: Starting for " << name << "..." << std::endl;
    
    try {
        // Store the function name and callback for later use
        std::cout << "RegisterNativeFunction: Storing function for later use" << std::endl;
        native_functions_[name] = callback;
        
        std::cout << "RegisterNativeFunction: Complete for " << name << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in RegisterNativeFunction: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in RegisterNativeFunction" << std::endl;
    }
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
    std::cout << "RegisterNativeModules: Starting..." << std::endl;
    
    try {
        // Enable native module registration
        std::cout << "RegisterNativeModules: Registering native modules..." << std::endl;
        
        // Register the fs module
        std::cout << "RegisterNativeModules: Registering fs module..." << std::endl;
        RegisterFsModule(this);
        
        // Register the http module
        std::cout << "RegisterNativeModules: Registering http module..." << std::endl;
        RegisterHttpModule(this);
        
        std::cout << "RegisterNativeModules: Complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in RegisterNativeModules: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in RegisterNativeModules" << std::endl;
    }
} 