/**
 * Test Script for Process Module in Tiny Node.js Runtime
 * 
 * This script tests various features of the process module:
 * - process.argv: Command-line arguments
 * - process.env: Environment variables
 * - process.cwd(): Current working directory
 * - process.exit(): Exit the process (commented out to avoid exiting the test)
 */

// Print a header
print("===== Process Module Test =====");

// Test process.argv
print("Testing process.argv:");
print("Command-line arguments:");
for (let i = 0; i < process.argv.length; i++) {
  print(`  argv[${i}] = ${process.argv[i]}`);
}

// Test process.env
print("\nTesting process.env:");
print("Environment variables (partial list):");
const envVars = ["PATH", "HOME", "USER", "SHELL"];
for (const name of envVars) {
  if (process.env[name]) {
    print(`  ${name} = ${process.env[name]}`);
  }
}

// Test process.cwd()
print("\nTesting process.cwd():");
print(`Current working directory: ${process.cwd()}`);

// Test process properties
print("\nTesting process properties:");
print(`Type of process: ${typeof process}`);
print(`Available process properties:`);
for (const prop in process) {
  const type = typeof process[prop];
  print(`  ${prop} (${type})`);
}

// NOTE: We don't test process.exit() as it would terminate the test
print("\nProcess exit test is skipped to avoid terminating the test.");

print("\n===== Process Module Test Complete ====="); 