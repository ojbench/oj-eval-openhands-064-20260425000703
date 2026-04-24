

#include <iostream>
#include <string>
#include <vector>
#include "sjtu_printf.hpp"

int main() {
    // Test the printf implementation with various format strings
    // This is a placeholder - the actual implementation will depend on the test cases
    
    // Example 1: String formatting
    std::string str = "Hello";
    sjtu::printf("String: %s\n", str);
    
    // Example 2: Integer formatting
    int num = 42;
    sjtu::printf("Number: %d\n", num);
    
    // Example 3: Unsigned integer formatting
    unsigned int unum = 100;
    sjtu::printf("Unsigned: %u\n", unum);
    
    // Example 4: Default formatting with %_
    sjtu::printf("Default int: %_\n", num);
    sjtu::printf("Default uint: %_\n", unum);
    sjtu::printf("Default string: %_\n", str);
    
    // Example 5: Vector formatting
    std::vector<int> vec = {1, 2, 3, 4, 5};
    sjtu::printf("Vector: %_\n", vec);
    
    // Example 6: Escaping %
    sjtu::printf("Percentage: %%100\n");
    
    return 0;
}

