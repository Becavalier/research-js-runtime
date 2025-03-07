#include <iostream>
#include "v8.h"

int main() {
    std::cout << "V8 version: " << v8::V8::GetVersion() << std::endl;
    return 0;
} 