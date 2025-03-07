#ifndef TINY_NODEJS_PROCESS_MODULE_H
#define TINY_NODEJS_PROCESS_MODULE_H

// Forward declaration
class Runtime;

/**
 * @brief Register the process module with the runtime
 * 
 * This function creates and registers the process module, which provides
 * access to process-related information and functionality, similar to
 * Node.js's process global object.
 * 
 * The process module exposes the following properties and methods to JavaScript:
 * - process.argv: Array of command-line arguments
 * - process.env: Object containing environment variables
 * - process.cwd(): Function that returns the current working directory
 * - process.exit(code): Function that exits the process with the specified code
 * 
 * @param runtime Pointer to the Runtime instance
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 */
void RegisterProcessModule(Runtime* runtime, int argc, char* argv[]);

#endif // TINY_NODEJS_PROCESS_MODULE_H 