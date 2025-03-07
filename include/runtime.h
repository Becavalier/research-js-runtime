#ifndef TINY_NODEJS_RUNTIME_H
#define TINY_NODEJS_RUNTIME_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include "v8.h"
#include "libplatform/libplatform.h"

// Forward declarations
class EventLoop;
class ModuleSystem;

/**
 * @brief Core runtime class for the tiny Node.js implementation
 * 
 * The Runtime class is responsible for:
 * - Initializing and managing the V8 JavaScript engine
 * - Executing JavaScript code
 * - Managing the event loop for asynchronous operations
 * - Handling the module system for code organization
 * - Providing an interface for native function registration
 */
class Runtime {
public:
    /**
     * @brief Initialize the V8 platform and JavaScript engine
     * 
     * This static method must be called before creating any Runtime instances.
     * It initializes the V8 platform and JavaScript engine.
     * 
     * @return true if initialization was successful, false otherwise
     */
    static bool Initialize();
    
    /**
     * @brief Shutdown the V8 platform and JavaScript engine
     * 
     * This static method should be called after all Runtime instances have been destroyed.
     * It cleans up the V8 platform and JavaScript engine resources.
     */
    static void Shutdown();
    
    /**
     * @brief Constructor for the Runtime class
     * 
     * Creates a new Runtime instance with its own V8 isolate, event loop, and module system.
     */
    Runtime();
    
    /**
     * @brief Destructor for the Runtime class
     * 
     * Cleans up resources used by the Runtime instance, including the event loop,
     * module system, and V8 isolate.
     */
    ~Runtime();
    
    /**
     * @brief Execute a JavaScript file
     * 
     * Reads the content of the specified file and executes it as JavaScript code.
     * 
     * @param filename Path to the JavaScript file to execute
     * @return true if execution was successful, false otherwise
     */
    bool ExecuteFile(const std::string& filename);
    
    /**
     * @brief Execute a JavaScript string
     * 
     * Executes the provided string as JavaScript code.
     * 
     * @param source JavaScript code to execute
     * @param source_name Optional name for the source (used in error messages)
     * @return true if execution was successful, false otherwise
     */
    bool ExecuteString(const std::string& source, const std::string& source_name = "");
    
    /**
     * @brief Register a native C++ function to be callable from JavaScript
     * 
     * @param name Name of the function in the JavaScript environment
     * @param callback C++ function to be called when the JavaScript function is invoked
     */
    void RegisterNativeFunction(const std::string& name, v8::FunctionCallback callback);
    
    /**
     * @brief Get the event loop instance
     * 
     * @return Pointer to the event loop
     */
    EventLoop* GetEventLoop() const;
    
    /**
     * @brief Get the module system instance
     * 
     * @return Pointer to the module system
     */
    ModuleSystem* GetModuleSystem() const;
    
    /**
     * @brief Get the V8 isolate instance
     * 
     * @return Pointer to the V8 isolate
     */
    v8::Isolate* GetIsolate() const;
    
    /**
     * @brief Schedule a task to be executed on the event loop
     * 
     * @param task Function to be executed
     */
    void ScheduleTask(std::function<void()> task);
    
    /**
     * @brief Schedule a task to be executed after a delay
     * 
     * @param task Function to be executed
     * @param delay_ms Delay in milliseconds
     * @return Task ID that can be used to cancel the task
     */
    uint64_t ScheduleDelayedTask(std::function<void()> task, uint64_t delay_ms);
    
    /**
     * @brief Cancel a previously scheduled delayed task
     * 
     * @param task_id ID of the task to cancel
     */
    void CancelDelayedTask(uint64_t task_id);
    
private:
    /**
     * @brief V8 platform instance (shared by all Runtime instances)
     */
    static std::unique_ptr<v8::Platform> platform_;
    
    /**
     * @brief V8 isolate instance (one per Runtime instance)
     */
    v8::Isolate* isolate_;
    
    /**
     * @brief Global object template for creating JavaScript contexts
     */
    v8::Global<v8::ObjectTemplate> global_template_;
    
    /**
     * @brief Event loop for handling asynchronous operations
     */
    std::unique_ptr<EventLoop> event_loop_;
    
    /**
     * @brief Module system for handling JavaScript modules
     */
    std::unique_ptr<ModuleSystem> module_system_;
    
    /**
     * @brief Map of native function names to callbacks
     */
    std::unordered_map<std::string, v8::FunctionCallback> native_functions_;
    
    /**
     * @brief Read a file into a string
     * 
     * @param filename Path to the file to read
     * @return Content of the file as a string
     */
    std::string ReadFile(const std::string& filename);
    
    /**
     * @brief Create a new V8 context with the global template
     * 
     * @return New V8 context
     */
    v8::Local<v8::Context> CreateContext();
    
    /**
     * @brief Setup global functions in the JavaScript environment
     * 
     * Registers built-in functions like print, setTimeout, etc.
     */
    void SetupGlobalFunctions();
    
    /**
     * @brief Register native modules in the JavaScript environment
     * 
     * Registers built-in modules like fs, http, process, etc.
     */
    void RegisterNativeModules();
};

#endif // TINY_NODEJS_RUNTIME_H 