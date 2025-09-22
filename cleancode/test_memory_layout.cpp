#include "include/shapes.h"
#include <iostream>
#include <cstring>

int main() {
    square s(3.0f);
    rectangle r(3.0f, 4.0f);
    triangle t(3.0f, 4.0f);
    circle c(3.0f);
    
    std::cout << "=== Memory Layout Analysis ===" << std::endl;
    std::cout << "sizeof(shape_base): " << sizeof(shape_base) << std::endl;
    std::cout << "sizeof(square): " << sizeof(square) << std::endl;
    std::cout << "sizeof(rectangle): " << sizeof(rectangle) << std::endl;
    std::cout << "sizeof(triangle): " << sizeof(triangle) << std::endl;
    std::cout << "sizeof(circle): " << sizeof(circle) << std::endl;
    std::cout << "sizeof(int): " << sizeof(int) << std::endl;
    std::cout << std::endl;
    
    // Test direct access to type_id
    std::cout << "=== Direct type_id access ===" << std::endl;
    std::cout << "square.type_id: " << s.type_id << std::endl;
    std::cout << "rectangle.type_id: " << r.type_id << std::endl;
    std::cout << "triangle.type_id: " << t.type_id << std::endl;
    std::cout << "circle.type_id: " << c.type_id << std::endl;
    std::cout << std::endl;
    
    // Test through base pointer - this should fail to compile if type_id is not accessible
    std::cout << "=== Base pointer access test ===" << std::endl;
    shape_base* shapes[] = {&s, &r, &t, &c};
    
    for (int i = 0; i < 4; i++) {
        std::cout << "Shape " << i << ":" << std::endl;
        std::cout << "  Address: " << shapes[i] << std::endl;
        
        // Let's examine the memory after the vtable pointer
        // The vtable pointer is typically the first 8 bytes (on 64-bit)
        char* ptr = reinterpret_cast<char*>(shapes[i]);
        ptr += sizeof(void*); // Skip vtable pointer
        
        // Try to read an int at this location (where type_id might be)
        int* potential_type_id = reinterpret_cast<int*>(ptr);
        std::cout << "  Potential type_id at offset " << sizeof(void*) << ": " << *potential_type_id << std::endl;
        
        // Check a few more locations
        for (int offset = 0; offset < 32; offset += 4) {
            int* test_ptr = reinterpret_cast<int*>(reinterpret_cast<char*>(shapes[i]) + offset);
            std::cout << "  Offset " << offset << ": " << *test_ptr << std::endl;
        }
        std::cout << std::endl;
    }
    
    return 0;
}
