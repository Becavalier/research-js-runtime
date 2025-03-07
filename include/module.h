#ifndef TINY_NODEJS_MODULE_H
#define TINY_NODEJS_MODULE_H

#include <string>
#include <unordered_map>
#include <memory>
#include "v8.h"

// Forward declaration
class Runtime;

/**
 * @brief Represents a JavaScript module
 * 
 * The Module class is responsible for loading and executing JavaScript modules.
 * It implements a simplified version of the CommonJS module system used in Node.js.
 */
class Module {
public:
    /**
     * @brief Constructor for the Module class
     * 
     * @param runtime Pointer to the Runtime instance
     * @param id Module identifier (e.g., './math')
     * @param filename Path to the module file
     */
    Module(Runtime* runtime, const std::string& id, const std::string& filename);
    
    /**
     * @brief Destructor for the Module class
     */
    ~Module();
    
    /**
     * @brief Load and execute the module
     * 
     * Reads the module file, wraps it in a function to create a module scope,
     * and executes it to populate the exports object.
     * 
     * @return true if the module was loaded successfully, false otherwise
     */
    bool Load();
    
    /**
     * @brief Get the module identifier
     * 
     * @return Module identifier
     */
    const std::string& GetId() const;
    
    /**
     * @brief Get the module filename
     * 
     * @return Path to the module file
     */
    const std::string& GetFilename() const;
    
    /**
     * @brief Get the module exports object
     * 
     * @param isolate V8 isolate instance
     * @return Module exports object
     */
    v8::Local<v8::Object> GetExports(v8::Isolate* isolate);
    
private:
    /**
     * @brief Pointer to the Runtime instance
     */
    Runtime* runtime_;
    
    /**
     * @brief Module identifier (e.g., './math')
     */
    std::string id_;
    
    /**
     * @brief Path to the module file
     */
    std::string filename_;
    
    /**
     * @brief Module exports object
     * 
     * This object contains all the values exported by the module.
     */
    v8::Global<v8::Object> exports_;
    
    /**
     * @brief Flag indicating whether the module has been loaded
     */
    bool loaded_;
};

/**
 * @brief Module system for managing JavaScript modules
 * 
 * The ModuleSystem class is responsible for:
 * - Loading and caching JavaScript modules
 * - Resolving module identifiers to filenames
 * - Managing native modules implemented in C++
 */
class ModuleSystem {
public:
    /**
     * @brief Constructor for the ModuleSystem class
     * 
     * @param runtime Pointer to the Runtime instance
     */
    ModuleSystem(Runtime* runtime);
    
    /**
     * @brief Destructor for the ModuleSystem class
     */
    ~ModuleSystem();
    
    /**
     * @brief Require a module (similar to require() in Node.js)
     * 
     * Loads the module if it hasn't been loaded yet, or returns the cached module.
     * 
     * @param module_id Module identifier (e.g., './math' or 'fs')
     * @return Module exports object
     */
    v8::Local<v8::Object> Require(const std::string& module_id);
    
    /**
     * @brief Register a native module implemented in C++
     * 
     * @param module_id Module identifier (e.g., 'fs')
     * @param exports Module exports object
     */
    void RegisterNativeModule(const std::string& module_id, v8::Local<v8::Object> exports);
    
    /**
     * @brief Get the Runtime instance
     * 
     * @return Pointer to the Runtime instance
     */
    Runtime* GetRuntime() const;
    
private:
    /**
     * @brief Pointer to the Runtime instance
     */
    Runtime* runtime_;
    
    /**
     * @brief Map of loaded JavaScript modules, indexed by module ID
     */
    std::unordered_map<std::string, std::shared_ptr<Module>> modules_;
    
    /**
     * @brief Map of native modules implemented in C++, indexed by module ID
     */
    std::unordered_map<std::string, v8::Global<v8::Object>> native_modules_;
    
    /**
     * @brief Resolve a module ID to a filename
     * 
     * @param module_id Module identifier (e.g., './math')
     * @return Path to the module file
     */
    std::string ResolveModuleId(const std::string& module_id);
};

/**
 * @brief Native implementation of the require() function
 * 
 * This function is exposed to JavaScript as the global require() function.
 * It delegates to the ModuleSystem::Require method.
 * 
 * @param args The JavaScript function arguments
 */
void Require(const v8::FunctionCallbackInfo<v8::Value>& args);

#endif // TINY_NODEJS_MODULE_H 