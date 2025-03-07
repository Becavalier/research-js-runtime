# 1. Introduction to Tiny Node.js

## What is Tiny Node.js?

Tiny Node.js is a lightweight JavaScript runtime environment inspired by Node.js. Unlike heavy-duty production environments, it's designed as a learning tool to understand how JavaScript runtimes work under the hood. This project demonstrates how to build a minimal yet functional runtime that can execute JavaScript code outside of a browser.

## Why Build a JavaScript Runtime?

JavaScript was originally designed to run in web browsers, but the ability to run JavaScript on servers and other non-browser environments has revolutionized web development, enabling:

- **Full-stack JavaScript**: Using the same language for both frontend and backend development
- **Event-driven programming**: Leveraging JavaScript's natural fit for event-based, non-blocking operations
- **Access to system resources**: Interacting with the file system, network, and other OS features
- **Package ecosystem**: Utilizing a vast ecosystem of reusable packages

By building a runtime from scratch, we gain deep insights into:
- How JavaScript engines like V8 integrate with system-level programming
- The mechanisms behind module systems, asynchronous I/O, and event loops
- The bridge between high-level JavaScript and low-level system operations

## Project Goals

Tiny Node.js aims to:

1. **Educational Value**: Provide a clear, understandable implementation of core Node.js concepts
2. **Minimalism**: Include only essential components to keep the codebase manageable
3. **Functionality**: Support basic JavaScript execution, file operations, and network capabilities
4. **Extensibility**: Allow for easy addition of new features and modules

## Architecture Overview

At its core, Tiny Node.js consists of several key components:

```
┌─────────────────────────────────────────────────────────┐
│                     Tiny Node.js                        │
│                                                         │
│  ┌───────────────┐    ┌───────────────┐    ┌──────────┐ │
│  │      V8       │    │    LibUV      │    │  Native  │ │
│  │ JavaScript    │◄───┤Event Loop &   │◄───┤ Modules  │ │
│  │   Engine      │    │   I/O         │    │          │ │
│  └───────┬───────┘    └───────────────┘    └──────────┘ │
│          │                                              │
│  ┌───────▼───────┐    ┌───────────────┐    ┌──────────┐ │
│  │   Runtime     │    │     Module    │    │JavaScript│ │
│  │     Core      │◄───┤     System    │◄───┤   Code   │ │
│  │               │    │               │    │          │ │
│  └───────────────┘    └───────────────┘    └──────────┘ │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Key Components:

1. **V8 JavaScript Engine**: Google's open-source JavaScript engine that compiles and executes JavaScript code.

2. **Runtime Core**: The central component that initializes V8, sets up the execution environment, and manages the runtime lifecycle.

3. **Module System**: Handles loading and caching JavaScript modules, similar to Node.js's CommonJS module system.

4. **LibUV Integration**: Provides event loop and asynchronous I/O operations across different operating systems.

5. **Native Modules**: C++ implementations of core functionality like file system operations, HTTP servers, and process information.

6. **Native Functions**: JavaScript functions implemented in C++ that bridge the gap between JavaScript and the underlying system.

## High-Level Flow

When Tiny Node.js executes a JavaScript file, the following happens:

1. The Runtime initializes the V8 engine
2. Native functions and modules are registered in the JavaScript environment
3. The event loop is created
4. The target JavaScript file is loaded and executed
5. The program runs until the event loop is empty (all callbacks processed)
6. The runtime shuts down and cleans up resources

## What Sets This Implementation Apart

While Tiny Node.js shares conceptual similarities with Node.js, it's simplified in several ways:

- **Focused Scope**: Only implements the most essential functionality
- **Simplified Module System**: Basic version of CommonJS module loading
- **Limited API Surface**: Only includes core modules like `fs`, `http`, and `process`
- **Clear Implementation**: Code prioritizes readability over optimization
- **Educational Structure**: Organized to facilitate learning rather than production performance

## Getting Started

The best way to understand Tiny Node.js is to explore both its code and its functionality. The following chapters will dive deep into each component, explaining their implementation details and the reasoning behind design decisions.

Let's begin by understanding V8, the JavaScript engine at the heart of our runtime.

[Next: V8 JavaScript Engine Basics →](02-v8-basics.md) 