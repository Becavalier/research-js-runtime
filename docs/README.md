# Tiny Node.js Implementation Guide

Welcome to the Tiny Node.js implementation guide! This comprehensive documentation aims to explain how this lightweight JavaScript runtime is built, helping you understand the inner workings of a Node.js-like runtime environment.

## Who Is This Guide For?

This guide is designed for:
- Developers with basic knowledge of C++ and JavaScript
- Students learning about runtime environments
- Anyone curious about how Node.js works under the hood

## Table of Contents

1. [Introduction](01-introduction.md)
   - Overview of the project
   - Goals and scope
   - Architecture at a glance

2. [V8 JavaScript Engine Basics](02-v8-basics.md)
   - What is V8?
   - Core concepts (Isolates, Contexts, Handles)
   - JavaScript execution model

3. [Project Structure](03-project-structure.md)
   - Directory organization
   - Key components
   - Build system

4. [Runtime Core Implementation](04-runtime-core.md)
   - The Runtime class
   - Initializing V8
   - Executing JavaScript

5. [JavaScript Native Functions](05-native-functions.md)
   - Bridging C++ and JavaScript
   - Implementing `print`, `setTimeout`, etc.
   - Function callbacks

6. [Module System](06-module-system.md)
   - Module loading mechanism
   - `require()` implementation
   - Native module registration

7. [Event Loop](07-event-loop.md)
   - LibUV integration
   - Asynchronous operations
   - Timers implementation

8. [File System Operations](08-fs-module.md)
   - File system module implementation
   - Reading and writing files
   - Path handling

9. [HTTP Server Implementation](09-http-module.md)
   - HTTP server basics
   - Request and response handling
   - Network operations

10. [Process Module](10-process-module.md)
    - Command-line arguments
    - Environment variables
    - Process information

11. [Testing and Debugging](11-testing-debugging.md)
    - Test scripts
    - Debugging techniques
    - Common issues

12. [Advanced Topics](12-advanced-topics.md)
    - Performance considerations
    - Memory management
    - Security aspects

13. [Building Your Own Features](13-extending.md)
    - Adding new native modules
    - Extending existing functionality
    - Best practices

## How to Use This Guide

Start by reading the introduction to get an overview of the project. Then, you can either follow the guide sequentially to build a thorough understanding of the system, or jump to specific topics of interest.

Each chapter is designed to be relatively independent, but concepts build upon each other as you progress through the guide.

Happy learning! 