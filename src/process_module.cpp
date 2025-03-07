#include "process_module.h"
#include "runtime.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <unistd.h>

extern char** environ;

// Register the process module
void RegisterProcessModule(Runtime* runtime, int argc, char* argv[]) {
    v8::Isolate* isolate = runtime->GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Create a new context
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);
    
    // Create the process module object
    v8::Local<v8::Object> process = v8::Object::New(isolate);
    
    // Add the argv array
    v8::Local<v8::Array> js_argv = v8::Array::New(isolate, argc);
    for (int i = 0; i < argc; i++) {
        js_argv->Set(context, i, v8::String::NewFromUtf8(isolate, argv[i]).ToLocalChecked()).Check();
    }
    process->Set(context, v8::String::NewFromUtf8(isolate, "argv").ToLocalChecked(), js_argv).Check();
    
    // Add the env object
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
    
    // Add the exit method
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
            
            // Exit the process
            exit(exit_code);
        }).ToLocalChecked()
    ).Check();
    
    // Add the cwd method
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
    
    // Register the process module
    runtime->GetModuleSystem()->RegisterNativeModule("process", process);
} 