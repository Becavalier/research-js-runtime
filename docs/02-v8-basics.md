# 2. V8 JavaScript Engine Basics

## What is V8?

V8 is Google's open-source JavaScript and WebAssembly engine, written in C++. It was originally designed for Google Chrome but is now used in many projects, including Node.js. V8:

- Compiles JavaScript directly to native machine code before execution
- Implements a high-performance garbage collector
- Conforms to the ECMAScript standard (the specification that JavaScript follows)
- Is designed to run standalone or embedded into any C++ application

For Tiny Node.js, V8 serves as the core that executes JavaScript code, providing the bridge between the high-level JavaScript code and the underlying system.

## Key V8 Concepts for Runtime Development

To work with V8 effectively, you need to understand several core concepts that form the foundation of our JavaScript runtime:

### Isolates

An **Isolate** represents a completely independent JavaScript runtime with its own heap memory. Think of it as a self-contained environment where JavaScript code runs.

```cpp
// Creating an isolate
v8::Isolate* isolate = v8::Isolate::New(create_params);

// Enter the isolate scope when working with it
v8::Isolate::Scope isolate_scope(isolate);
```

Key points about Isolates:
- Only one thread may access an Isolate at a time
- Each Isolate has its own garbage collector
- Different Isolates cannot share JavaScript objects directly
- When building a runtime, you typically use a single Isolate for the entire application

### Handles

**Handles** are pointers to JavaScript objects in V8's heap. They come in several varieties:

1. **Local Handles** (`v8::Local<T>`): Short-lived handles that are automatically freed when their scope is exited.

```cpp
// Create a handle scope for managing local handles
v8::HandleScope handle_scope(isolate);

// Create a new local string
v8::Local<v8::String> local_string = v8::String::NewFromUtf8(isolate, "Hello World").ToLocalChecked();
```

2. **Persistent Handles** (`v8::Persistent<T>`): Long-lived handles that survive beyond the current scope.

```cpp
// Create a persistent handle from a local handle
v8::Persistent<v8::Object> persistent_obj;
persistent_obj.Reset(isolate, local_obj);

// When done with it, you must manually free it
persistent_obj.Reset();
```

Handles are crucial for memory management. V8's garbage collector needs to know which objects are still in use, and handles provide this information.

### Contexts

A **Context** represents a JavaScript execution environment with its own global object and set of built-in objects and functions.

```cpp
// Create a new context
v8::Local<v8::Context> context = v8::Context::New(isolate);

// Enter the context when you want to execute code in it
v8::Context::Scope context_scope(context);
```

Key points about Contexts:
- You can have multiple contexts in a single Isolate
- Each context has its own global object and built-in functions
- Contexts can share JavaScript values if they're in the same Isolate
- In Tiny Node.js, we create a new context for each JavaScript file execution

### Handling Scripts

To execute JavaScript code in V8, you compile it into a script and then run it:

```cpp
// Create a string containing JavaScript code
v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, "console.log('Hello World')").ToLocalChecked();

// Compile the script
v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();

// Run the script
script->Run(context);
```

### Templates

**Templates** are blueprints for JavaScript functions and objects:

1. **Function Templates** define the structure of a JavaScript function:

```cpp
// Create a function template with a C++ callback
v8::Local<v8::FunctionTemplate> func_template = v8::FunctionTemplate::New(isolate, MyCallback);

// Create a JavaScript function from the template
v8::Local<v8::Function> func = func_template->GetFunction(context).ToLocalChecked();
```

2. **Object Templates** define the structure of a JavaScript object:

```cpp
// Create an object template
v8::Local<v8::ObjectTemplate> obj_template = v8::ObjectTemplate::New(isolate);

// Set a property with a C++ accessor
obj_template->SetAccessor(
    v8::String::NewFromUtf8(isolate, "myProperty").ToLocalChecked(),
    GetterCallback,
    SetterCallback
);

// Create a JavaScript object from the template
v8::Local<v8::Object> obj = obj_template->NewInstance(context).ToLocalChecked();
```

Templates are essential for exposing C++ functionality to JavaScript.

## The V8 Execution Model

Understanding V8's execution model helps you build a better JavaScript runtime:

1. **Compilation**: V8 compiles JavaScript to machine code instead of interpreting it, allowing for faster execution.

2. **Just-In-Time (JIT) Compilation**: V8 compiles code at runtime, optimizing hot functions (frequently executed code) for better performance.

3. **Hidden Classes**: V8 creates hidden classes behind the scenes to optimize property access on JavaScript objects.

4. **Garbage Collection**: V8 automatically frees memory that's no longer being used, using a generational garbage collector that treats new and old objects differently for efficiency.

## How Tiny Node.js Uses V8

In our implementation:

1. We initialize V8 when the runtime starts
2. We create a single Isolate for the entire runtime
3. We create a new Context for each JavaScript file execution
4. We register native functions and modules in the Context
5. We compile and run JavaScript code, processing the results
6. We use Function Templates to bridge between C++ and JavaScript
7. We properly manage handles to prevent memory leaks

Here's a simplified overview of our V8 integration:

```cpp
// Initialize V8
v8::V8::InitializeICUDefaultLocation(argv[0]);
v8::V8::InitializeExternalStartupData(argv[0]);
std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
v8::V8::InitializePlatform(platform.get());
v8::V8::Initialize();

// Create an Isolate
v8::Isolate::CreateParams create_params;
create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
v8::Isolate* isolate = v8::Isolate::New(create_params);

// Within the Isolate, create a Context
v8::Isolate::Scope isolate_scope(isolate);
v8::HandleScope handle_scope(isolate);
v8::Local<v8::Context> context = v8::Context::New(isolate);
v8::Context::Scope context_scope(context);

// Register functions in the global object
v8::Local<v8::Object> global = context->Global();
global->Set(
    context,
    v8::String::NewFromUtf8(isolate, "print").ToLocalChecked(),
    v8::FunctionTemplate::New(isolate, PrintCallback)->GetFunction(context).ToLocalChecked()
).Check();

// Execute JavaScript
const char* js_code = "print('Hello from V8!');";
v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, js_code).ToLocalChecked();
v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
script->Run(context);

// Cleanup
isolate->Dispose();
v8::V8::Dispose();
v8::V8::ShutdownPlatform();
```

## Common Pitfalls When Working with V8

1. **Handle Scopes**: Forgetting to create handle scopes can lead to memory leaks.

2. **Context Switching**: Always ensure you're in the correct context when executing JavaScript.

3. **Error Handling**: V8 uses a try-catch mechanism for JavaScript exceptions, which must be properly handled.

4. **Memory Management**: Long-lived objects must be stored in persistent handles, which must be manually freed.

5. **Thread Safety**: V8 is not thread-safe; an Isolate can only be accessed by one thread at a time.

By understanding these V8 concepts, you can effectively build and maintain the core of a JavaScript runtime.

[← Previous: Introduction](01-introduction.md) | [Next: Project Structure →](03-project-structure.md) 