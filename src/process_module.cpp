#include "process_module.h"
#include "runtime.h"
#include "module.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <unistd.h>
#include <sys/utsname.h>

extern char** environ;

// Register the process module
void RegisterProcessModule(Runtime* runtime, int argc, char* argv[]) {
    std::cout << "RegisterProcessModule: Starting..." << std::endl;
    
    try {
        v8::Isolate* isolate = runtime->GetIsolate();
        std::cout << "RegisterProcessModule: Got isolate" << std::endl;
        
        // Create a handle scope
        v8::HandleScope scope(isolate);
        std::cout << "RegisterProcessModule: Created handle scope" << std::endl;
        
        // Create a new context for module initialization
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);
        std::cout << "RegisterProcessModule: Created and entered context" << std::endl;
        
        // Create the process module object
        v8::Local<v8::Object> process = v8::Object::New(isolate);
        std::cout << "RegisterProcessModule: Created process object" << std::endl;
        
        // Add the argv array
        std::cout << "RegisterProcessModule: Adding argv..." << std::endl;
        v8::Local<v8::Array> js_argv = v8::Array::New(isolate, argc);
        for (int i = 0; i < argc; i++) {
            js_argv->Set(context, i, v8::String::NewFromUtf8(isolate, argv[i]).ToLocalChecked()).Check();
        }
        process->Set(context, v8::String::NewFromUtf8(isolate, "argv").ToLocalChecked(), js_argv).Check();
        
        // Add the env object
        std::cout << "RegisterProcessModule: Adding env..." << std::endl;
        v8::Local<v8::Object> env = v8::Object::New(isolate);
        
        // Get the environment variables
        char** env_vars = environ;
        while (*env_vars) {
            std::string env_var = *env_vars;
            size_t pos = env_var.find('=');
            if (pos != std::string::npos) {
                std::string name = env_var.substr(0, pos);
                std::string value = env_var.substr(pos + 1);
                env->Set(context, 
                    v8::String::NewFromUtf8(isolate, name.c_str()).ToLocalChecked(),
                    v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked()
                ).Check();
            }
            env_vars++;
        }
        process->Set(context, v8::String::NewFromUtf8(isolate, "env").ToLocalChecked(), env).Check();
        
        // Add version information
        std::cout << "RegisterProcessModule: Adding version info..." << std::endl;
        process->Set(context, 
            v8::String::NewFromUtf8(isolate, "version").ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, "1.0.0").ToLocalChecked()
        ).Check();
        
        // Create version object similar to Node.js
        v8::Local<v8::Object> versions = v8::Object::New(isolate);
        versions->Set(context,
            v8::String::NewFromUtf8(isolate, "tiny_node").ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, "1.0.0").ToLocalChecked()
        ).Check();
        
        // Add V8 version
        versions->Set(context,
            v8::String::NewFromUtf8(isolate, "v8").ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, v8::V8::GetVersion()).ToLocalChecked()
        ).Check();
        
        process->Set(context, 
            v8::String::NewFromUtf8(isolate, "versions").ToLocalChecked(), 
            versions
        ).Check();
        
        // Add platform information
        std::cout << "RegisterProcessModule: Adding platform info..." << std::endl;
        struct utsname system_info;
        if (uname(&system_info) == 0) {
            process->Set(context, 
                v8::String::NewFromUtf8(isolate, "platform").ToLocalChecked(),
                v8::String::NewFromUtf8(isolate, system_info.sysname).ToLocalChecked()
            ).Check();
            
            process->Set(context, 
                v8::String::NewFromUtf8(isolate, "arch").ToLocalChecked(),
                v8::String::NewFromUtf8(isolate, system_info.machine).ToLocalChecked()
            ).Check();
        } else {
            process->Set(context, 
                v8::String::NewFromUtf8(isolate, "platform").ToLocalChecked(),
                v8::String::NewFromUtf8(isolate, "unknown").ToLocalChecked()
            ).Check();
            
            process->Set(context, 
                v8::String::NewFromUtf8(isolate, "arch").ToLocalChecked(),
                v8::String::NewFromUtf8(isolate, "unknown").ToLocalChecked()
            ).Check();
        }
        
        // Add the exit method
        std::cout << "RegisterProcessModule: Adding exit function..." << std::endl;
        process->Set(context, 
            v8::String::NewFromUtf8(isolate, "exit").ToLocalChecked(),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                v8::Isolate* isolate = args.GetIsolate();
                v8::HandleScope scope(isolate);
                
                // Get the exit code
                int exit_code = 0;
                if (args.Length() > 0 && args[0]->IsNumber()) {
                    exit_code = args[0]->Int32Value(isolate->GetCurrentContext()).FromJust();
                }
                
                std::cout << "Process exit called with code: " << exit_code << std::endl;
                
                // Exit the process
                exit(exit_code);
            }).ToLocalChecked()
        ).Check();
        
        // Add the cwd method
        std::cout << "RegisterProcessModule: Adding cwd function..." << std::endl;
        process->Set(context, 
            v8::String::NewFromUtf8(isolate, "cwd").ToLocalChecked(),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                v8::Isolate* isolate = args.GetIsolate();
                v8::HandleScope scope(isolate);
                
                // Get the current working directory
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, cwd).ToLocalChecked());
                } else {
                    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, "").ToLocalChecked());
                }
            }).ToLocalChecked()
        ).Check();
        
        // Register the process module as a global object
        std::cout << "RegisterProcessModule: Registering as global object..." << std::endl;
        runtime->GetModuleSystem()->RegisterNativeModule("process", process);
        
        // We don't need to add it to the global object here, that will be handled in CreateContext
        
        std::cout << "RegisterProcessModule: Complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in RegisterProcessModule: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in RegisterProcessModule" << std::endl;
    }
} 