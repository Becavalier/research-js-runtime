#include "http_module.h"
#include "runtime.h"
#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>

// HTTP server class
class HttpServer {
public:
    // Constructor
    HttpServer(Runtime* runtime) : runtime_(runtime), running_(false), next_connection_id_(1) {
    }
    
    // Destructor
    ~HttpServer() {
        Stop();
    }
    
    // Start the server
    bool Start(int port) {
        if (running_) {
            return true;
        }
        
        // Create a socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options" << std::endl;
            close(server_fd_);
            return false;
        }
        
        // Bind the socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind socket" << std::endl;
            close(server_fd_);
            return false;
        }
        
        // Listen for connections
        if (listen(server_fd_, 10) < 0) {
            std::cerr << "Failed to listen on socket" << std::endl;
            close(server_fd_);
            return false;
        }
        
        // Start the accept thread
        running_ = true;
        accept_thread_ = std::thread(&HttpServer::AcceptConnections, this);
        
        return true;
    }
    
    // Stop the server
    void Stop() {
        if (!running_) {
            return;
        }
        
        running_ = false;
        
        // Close the server socket
        close(server_fd_);
        
        // Join the accept thread
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        
        // Close all client connections
        for (auto& pair : connections_) {
            close(pair.second);
        }
        connections_.clear();
    }
    
    // Register a request handler
    void RegisterRequestHandler(v8::Local<v8::Function> handler) {
        v8::Isolate* isolate = runtime_->GetIsolate();
        request_handler_.Reset(isolate, handler);
    }
    
private:
    // Runtime instance
    Runtime* runtime_;
    
    // Server socket
    int server_fd_;
    
    // Running flag
    std::atomic<bool> running_;
    
    // Accept thread
    std::thread accept_thread_;
    
    // Client connections
    std::unordered_map<int, int> connections_;
    
    // Next connection ID
    std::atomic<int> next_connection_id_;
    
    // Request handler
    v8::Global<v8::Function> request_handler_;
    
    // Mutex for connections
    std::mutex connections_mutex_;
    
    // Accept connections
    void AcceptConnections() {
        while (running_) {
            // Accept a connection
            struct sockaddr_in address;
            socklen_t addrlen = sizeof(address);
            int client_fd = accept(server_fd_, (struct sockaddr*)&address, &addrlen);
            
            if (client_fd < 0) {
                if (running_) {
                    std::cerr << "Failed to accept connection" << std::endl;
                }
                continue;
            }
            
            // Add the connection to the map
            int connection_id = next_connection_id_++;
            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                connections_[connection_id] = client_fd;
            }
            
            // Handle the connection in a new thread
            std::thread(&HttpServer::HandleConnection, this, connection_id, client_fd).detach();
        }
    }
    
    // Handle a connection
    void HandleConnection(int connection_id, int client_fd) {
        // Read the request
        char buffer[1024] = {0};
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            // Close the connection
            close(client_fd);
            
            // Remove the connection from the map
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.erase(connection_id);
            
            return;
        }
        
        // Parse the request
        std::string request(buffer, bytes_read);
        std::string method;
        std::string path;
        std::string version;
        
        // Extract the request line
        size_t pos = request.find("\r\n");
        if (pos != std::string::npos) {
            std::string request_line = request.substr(0, pos);
            
            // Extract the method, path, and version
            size_t method_end = request_line.find(" ");
            if (method_end != std::string::npos) {
                method = request_line.substr(0, method_end);
                
                size_t path_end = request_line.find(" ", method_end + 1);
                if (path_end != std::string::npos) {
                    path = request_line.substr(method_end + 1, path_end - method_end - 1);
                    version = request_line.substr(path_end + 1);
                }
            }
        }
        
        // Create the request object
        v8::Isolate* isolate = runtime_->GetIsolate();
        v8::HandleScope handle_scope(isolate);
        
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Context::Scope context_scope(context);
        
        v8::Local<v8::Object> req = v8::Object::New(isolate);
        req->Set(context, v8::String::NewFromUtf8(isolate, "method").ToLocalChecked(), 
                v8::String::NewFromUtf8(isolate, method.c_str()).ToLocalChecked()).Check();
        req->Set(context, v8::String::NewFromUtf8(isolate, "url").ToLocalChecked(), 
                v8::String::NewFromUtf8(isolate, path.c_str()).ToLocalChecked()).Check();
        req->Set(context, v8::String::NewFromUtf8(isolate, "httpVersion").ToLocalChecked(), 
                v8::String::NewFromUtf8(isolate, version.c_str()).ToLocalChecked()).Check();
        
        // Create the response object
        v8::Local<v8::Object> res = v8::Object::New(isolate);
        
        // Add the writeHead method
        res->Set(context, v8::String::NewFromUtf8(isolate, "writeHead").ToLocalChecked(),
                v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                    // This is a placeholder for the writeHead method
                    // In a real implementation, we would store the status code and headers
                }).ToLocalChecked()).Check();
        
        // Add the end method
        res->Set(context, v8::String::NewFromUtf8(isolate, "end").ToLocalChecked(),
                v8::Function::New(context, [this, client_fd, connection_id](const v8::FunctionCallbackInfo<v8::Value>& args) {
                    v8::Isolate* isolate = args.GetIsolate();
                    v8::HandleScope scope(isolate);
                    
                    // Get the response body
                    std::string body;
                    if (args.Length() > 0) {
                        v8::String::Utf8Value str(isolate, args[0]);
                        body = *str;
                    }
                    
                    // Create the response
                    std::string response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/plain\r\n";
                    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
                    response += "\r\n";
                    response += body;
                    
                    // Send the response
                    send(client_fd, response.c_str(), response.length(), 0);
                    
                    // Close the connection
                    close(client_fd);
                    
                    // Remove the connection from the map
                    std::lock_guard<std::mutex> lock(this->connections_mutex_);
                    this->connections_.erase(connection_id);
                }).ToLocalChecked()).Check();
        
        // Call the request handler
        if (!request_handler_.IsEmpty()) {
            v8::Local<v8::Function> handler = v8::Local<v8::Function>::New(isolate, request_handler_);
            v8::Local<v8::Value> args[2] = { req, res };
            handler->Call(context, context->Global(), 2, args);
        } else {
            // No handler registered, send a default response
            std::string response = "HTTP/1.1 404 Not Found\r\n";
            response += "Content-Type: text/plain\r\n";
            response += "Content-Length: 9\r\n";
            response += "\r\n";
            response += "Not Found";
            
            // Send the response
            send(client_fd, response.c_str(), response.length(), 0);
            
            // Close the connection
            close(client_fd);
            
            // Remove the connection from the map
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.erase(connection_id);
        }
    }
};

// HTTP server map
static std::unordered_map<int, std::shared_ptr<HttpServer>> http_servers;
static int next_server_id = 1;
static std::mutex http_servers_mutex;

// Native createServer function
void CreateServer(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Check arguments
    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
        return;
    }
    
    // Get the request handler
    v8::Local<v8::Function> handler = v8::Local<v8::Function>::Cast(args[0]);
    
    // Get the runtime from the external data
    Runtime* runtime = static_cast<Runtime*>(args.Data().As<v8::External>()->Value());
    
    // Create a new HTTP server
    std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>(runtime);
    
    // Register the request handler
    server->RegisterRequestHandler(handler);
    
    // Add the server to the map
    int server_id;
    {
        std::lock_guard<std::mutex> lock(http_servers_mutex);
        server_id = next_server_id++;
        http_servers[server_id] = server;
    }
    
    // Create the server object
    v8::Local<v8::Object> server_obj = v8::Object::New(isolate);
    
    // Add the listen method
    server_obj->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "listen").ToLocalChecked(),
            v8::Function::New(isolate->GetCurrentContext(), [server_id](const v8::FunctionCallbackInfo<v8::Value>& args) {
                v8::Isolate* isolate = args.GetIsolate();
                v8::HandleScope scope(isolate);
                
                // Check arguments
                if (args.Length() < 1 || !args[0]->IsNumber()) {
                    isolate->ThrowException(v8::Exception::TypeError(
                        v8::String::NewFromUtf8(isolate, "Invalid arguments").ToLocalChecked()));
                    return;
                }
                
                // Get the port
                int port = args[0]->Int32Value(isolate->GetCurrentContext()).FromJust();
                
                // Get the server
                std::shared_ptr<HttpServer> server;
                {
                    std::lock_guard<std::mutex> lock(http_servers_mutex);
                    auto it = http_servers.find(server_id);
                    if (it != http_servers.end()) {
                        server = it->second;
                    }
                }
                
                if (!server) {
                    isolate->ThrowException(v8::Exception::Error(
                        v8::String::NewFromUtf8(isolate, "Server not found").ToLocalChecked()));
                    return;
                }
                
                // Start the server
                if (!server->Start(port)) {
                    isolate->ThrowException(v8::Exception::Error(
                        v8::String::NewFromUtf8(isolate, "Failed to start server").ToLocalChecked()));
                    return;
                }
                
                // Return the server object
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked()).Check();
    
    // Return the server object
    args.GetReturnValue().Set(server_obj);
}

// Register the http module
void RegisterHttpModule(Runtime* runtime) {
    v8::Isolate* isolate = runtime->GetIsolate();
    v8::HandleScope scope(isolate);
    
    // Create a new context
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);
    
    // Create the http module object
    v8::Local<v8::Object> http = v8::Object::New(isolate);
    
    // Add the createServer function
    http->Set(context, 
        v8::String::NewFromUtf8(isolate, "createServer").ToLocalChecked(),
        v8::Function::New(context, CreateServer, v8::External::New(isolate, runtime)).ToLocalChecked()
    ).Check();
    
    // Register the http module
    runtime->GetModuleSystem()->RegisterNativeModule("http", http);
} 