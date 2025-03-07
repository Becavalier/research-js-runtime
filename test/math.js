// Math module

// Add two numbers
exports.add = function(a, b) {
    return a + b;
};

// Subtract two numbers
exports.subtract = function(a, b) {
    return a - b;
};

// Multiply two numbers
exports.multiply = function(a, b) {
    return a * b;
};

// Divide two numbers
exports.divide = function(a, b) {
    if (b === 0) {
        throw new Error("Division by zero");
    }
    return a / b;
};

// Print a message when the module is loaded
print("Math module loaded"); 