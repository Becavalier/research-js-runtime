#include "http_module.h"
#include "runtime.h"
#include "module.h"
#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

// Simple HTTP server implementation (mock)
class SimpleHttpServer {
public:
    SimpleHttpServer() : next_request_id_(1) {}
    
    bool Start(int port) {
        std::cout << "HTTP server starting on port " << port << std::endl;
        return true;
    }
    
    void Stop() {
        std::cout << "HTTP server stopping" << std::endl;
    }
    
    void HandleRequest(v8::Isolate* isolate, v8::Local<v8::Function> callback) {
        v8::HandleScope scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        
        // Create a mock request object
        v8::Local<v8::Object> req = v8::Object::New(isolate);
        req->Set(context, 
            v8::String::NewFromUtf8(isolate, "method").ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, "GET").ToLocalChecked()).Check();
        req->Set(context, 
            v8::String::NewFromUtf8(isolate, "url").ToLocalChecked(),
            v8::String::NewFromUtf8(isolate, "/").ToLocalChecked()).Check();
        
        // Create a mock response object
        v8::Local<v8::Object> res = v8::Object::New(isolate);
        
        // Add writeHead method
        res->Set(context,
            v8::String::NewFromUtf8(isolate, "writeHead").ToLocalChecked(),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                v8::Isolate* isolate = args.GetIsolate();
                if (args.Length() >= 1 && args[0]->IsNumber()) {
                    int status = args[0]->Int32Value(isolate->GetCurrentContext()).FromJust();
                    std::cout << "Response status: " << status << std::endl;
                }
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked()).Check();
        
        // Add end method
        res->Set(context,
            v8::String::NewFromUtf8(isolate, "end").ToLocalChecked(),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                v8::Isolate* isolate = args.GetIsolate();
                if (args.Length() >= 1 && args[0]->IsString()) {
                    v8::String::Utf8Value body(isolate, args[0]);
                    std::cout << "Response body: " << *body << std::endl;
                }
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked()).Check();
        
        // Call the callback with req and res
        v8::Local<v8::Value> argv[2] = { req, res };
        v8::MaybeLocal<v8::Value> result = callback->Call(context, context->Global(), 2, argv);
        if (result.IsEmpty()) {
            std::cerr << "Error calling request handler" << std::endl;
        }
    }
    
private:
    int next_request_id_;
};

// Global map of servers
static std::unordered_map<int, std::shared_ptr<SimpleHttpServer>> http_servers;
static int next_server_id = 1;

// CreateServer function
void CreateServer(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the callback function
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[0]);
    
    // Create a new HTTP server
    std::shared_ptr<SimpleHttpServer> server = std::make_shared<SimpleHttpServer>();
    
    // Get a new server ID
    int server_id = next_server_id++;
    http_servers[server_id] = server;
    
    // Create a server object to return
    v8::Local<v8::Object> server_obj = v8::Object::New(isolate);
    
    // Store the server ID and callback
    server_obj->Set(context, 
        v8::String::NewFromUtf8(isolate, "_serverId").ToLocalChecked(),
        v8::Integer::New(isolate, server_id)).Check();
    
    server_obj->Set(context, 
        v8::String::NewFromUtf8(isolate, "_callback").ToLocalChecked(),
        callback).Check();
    
    // Add the listen method
    server_obj->Set(context,
        v8::String::NewFromUtf8(isolate, "listen").ToLocalChecked(),
        v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* isolate = args.GetIsolate();
            v8::HandleScope scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            
            // Get the server object (this)
            v8::Local<v8::Object> server_obj = args.This();
            
            // Get the server ID
            v8::Local<v8::Value> server_id_val = server_obj->Get(context, 
                v8::String::NewFromUtf8(isolate, "_serverId").ToLocalChecked()).ToLocalChecked();
            int server_id = server_id_val->Int32Value(context).FromJust();
            
            // Get the callback function
            v8::Local<v8::Value> callback_val = server_obj->Get(context, 
                v8::String::NewFromUtf8(isolate, "_callback").ToLocalChecked()).ToLocalChecked();
            v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(callback_val);
            
            // Check arguments
            if (args.Length() < 1 || !args[0]->IsNumber()) {
                isolate->ThrowException(v8::Exception::TypeError(
                    v8::String::NewFromUtf8(isolate, "Port number required").ToLocalChecked()));
                return;
            }
            
            // Get the port
            int port = args[0]->Int32Value(context).FromJust();
            
            // Find the server
            auto it = http_servers.find(server_id);
            if (it == http_servers.end()) {
                isolate->ThrowException(v8::Exception::Error(
                    v8::String::NewFromUtf8(isolate, "Server not found").ToLocalChecked()));
                return;
            }
            
            // Start the server
            it->second->Start(port);
            
            // Simulate a request (for testing)
            it->second->HandleRequest(isolate, callback);
            
            // If there's a callback, call it
            if (args.Length() >= 2 && args[1]->IsFunction()) {
                v8::Local<v8::Function> listen_callback = v8::Local<v8::Function>::Cast(args[1]);
                v8::MaybeLocal<v8::Value> result = listen_callback->Call(context, server_obj, 0, nullptr);
                if (result.IsEmpty()) {
                    std::cerr << "Error calling listen callback" << std::endl;
                }
            }
            
            // Return this for chaining
            args.GetReturnValue().Set(server_obj);
        }).ToLocalChecked()).Check();
    
    // Add the close method
    server_obj->Set(context,
        v8::String::NewFromUtf8(isolate, "close").ToLocalChecked(),
        v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* isolate = args.GetIsolate();
            v8::HandleScope scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            
            // Get the server object (this)
            v8::Local<v8::Object> server_obj = args.This();
            
            // Get the server ID
            v8::Local<v8::Value> server_id_val = server_obj->Get(context, 
                v8::String::NewFromUtf8(isolate, "_serverId").ToLocalChecked()).ToLocalChecked();
            int server_id = server_id_val->Int32Value(context).FromJust();
            
            // Find the server
            auto it = http_servers.find(server_id);
            if (it == http_servers.end()) {
                isolate->ThrowException(v8::Exception::Error(
                    v8::String::NewFromUtf8(isolate, "Server not found").ToLocalChecked()));
                return;
            }
            
            // Stop the server
            it->second->Stop();
            
            // Remove the server from the map
            http_servers.erase(it);
            
            // Return this for chaining
            args.GetReturnValue().Set(server_obj);
        }).ToLocalChecked()).Check();
    
    // Return the server object
    args.GetReturnValue().Set(server_obj);
}

// Register the http module
void RegisterHttpModule(Runtime* runtime) {
    std::cout << "RegisterHttpModule: Starting..." << std::endl;
    
    try {
        v8::Isolate* isolate = runtime->GetIsolate();
        std::cout << "RegisterHttpModule: Got isolate" << std::endl;
        
        // Create a handle scope
        v8::HandleScope scope(isolate);
        std::cout << "RegisterHttpModule: Created handle scope" << std::endl;
        
        // Create a new context for module initialization
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);
        std::cout << "RegisterHttpModule: Created and entered context" << std::endl;
        
        // Create the http module object
        v8::Local<v8::Object> http = v8::Object::New(isolate);
        std::cout << "RegisterHttpModule: Created http object" << std::endl;
        
        // Add the createServer function
        std::cout << "RegisterHttpModule: Adding createServer function..." << std::endl;
        v8::Local<v8::Function> createServer = v8::Function::New(
            context, 
            CreateServer
        ).ToLocalChecked();
        http->Set(
            context,
            v8::String::NewFromUtf8(isolate, "createServer").ToLocalChecked(),
            createServer
        ).Check();
        
        // Register the http module
        std::cout << "RegisterHttpModule: Registering module with ModuleSystem..." << std::endl;
        runtime->GetModuleSystem()->RegisterNativeModule("http", http);
        
        std::cout << "RegisterHttpModule: Complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in RegisterHttpModule: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in RegisterHttpModule" << std::endl;
    }
} 