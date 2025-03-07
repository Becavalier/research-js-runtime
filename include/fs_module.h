#ifndef TINY_NODEJS_FS_MODULE_H
#define TINY_NODEJS_FS_MODULE_H

// Forward declaration
class Runtime;

/**
 * @brief Register the file system module with the runtime
 * 
 * This function creates and registers the fs module, which provides
 * file system operations, similar to Node.js's fs module.
 * 
 * The fs module exposes the following functionality to JavaScript:
 * - fs.readFile(path): Reads the content of a file
 * - fs.writeFile(path, data): Writes data to a file
 * - fs.exists(path): Checks if a file or directory exists
 * 
 * Note: This is a simplified version of Node.js's fs module and does not
 * include all the functionality or the asynchronous versions of these methods.
 * 
 * @param runtime Pointer to the Runtime instance
 */
void RegisterFsModule(Runtime* runtime);

#endif // TINY_NODEJS_FS_MODULE_H 