#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include "shapes.h"



// Declarations from other files
f32 TotalAreaVTBL(u32 ShapeCount, shape_base **Shapes);
f32 TotalAreaVTBL4(u32 ShapeCount, shape_base **Shapes);
f32 CornerAreaVTBL(u32 ShapeCount, shape_base **Shapes);
f32 CornerAreaVTBL4(u32 ShapeCount, shape_base **Shapes);

// Optimized buffer-based versions
f32 TotalAreaCollector(AreaCollector& acollector);
f32 CornerAreaCollector(OptimizedCornerCollector& collector);

// Buffer traversal optimized versions
f32 TotalAreaOptVTBL(u32 ShapeCount, char* buffer);
f32 TotalAreaOptVTBL4(u32 ShapeCount, char* buffer);
f32 CornerAreaOptVTBL(u32 ShapeCount, char* buffer);
f32 CornerAreaOptVTBL4(u32 ShapeCount, char* buffer);

f32 TotalAreaSwitch(u32 ShapeCount, shape_union* Shapes);
f32 TotalAreaSwitch4(u32 ShapeCount, shape_union* Shapes);
f32 CornerAreaSwitch(u32 ShapeCount, shape_union* Shapes);
f32 CornerAreaSwitch4(u32 ShapeCount, shape_union* Shapes);

f32 TotalAreaUnion(u32 ShapeCount, shape_union* Shapes);
f32 TotalAreaUnion4(u32 ShapeCount, shape_union* Shapes);
f32 CornerAreaUnion(u32 ShapeCount, shape_union* Shapes);
f32 CornerAreaUnion4(u32 ShapeCount, shape_union* Shapes);

constexpr u32 N = 1000000;
constexpr u32 COUNT = 100;

void bench(const char* name, f32(*func)(u32, void*), u32 count, void* shapes) {
    auto start = std::chrono::high_resolution_clock::now();
    f32 result = 0.0f;
    for (u32 i = 0; i < COUNT; ++i) {
        result = func(count, shapes);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << name << ": " << ms / COUNT << " ms avg (" << COUNT << " runs), result = " << result << std::endl;
}

// Specialized bench function for TotalAreaCollector
void bench_total_collector(const char* name, f32(*func)(AreaCollector&), AreaCollector& collector) {
    auto start = std::chrono::high_resolution_clock::now();
    f32 result = 0.0f;
    for (u32 i = 0; i < COUNT; ++i) {
        result = func(collector);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << name << ": " << ms / COUNT << " ms avg (" << COUNT << " runs), result = " << result << std::endl;
}

// Specialized bench function for OptimizedCornerCollector
void bench_optimized_corner_collector(const char* name, f32(*func)(OptimizedCornerCollector&), 
                                      OptimizedCornerCollector& collector) {
    auto start = std::chrono::high_resolution_clock::now();
    f32 result = 0.0f;
    for (u32 i = 0; i < COUNT; ++i) {
        result = func(collector);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << name << ": " << ms / COUNT << " ms avg (" << COUNT << " runs), result = " << result << std::endl;
}



// Wrappers for function pointer compatibility
f32 vtbl_area(u32 count, void* shapes) { return TotalAreaVTBL(count, (shape_base**)shapes); }
f32 vtbl_area4(u32 count, void* shapes) { return TotalAreaVTBL4(count, (shape_base**)shapes); }
f32 vtbl_corner(u32 count, void* shapes) { return CornerAreaVTBL(count, (shape_base**)shapes); }
f32 vtbl_corner4(u32 count, void* shapes) { return CornerAreaVTBL4(count, (shape_base**)shapes); }

f32 switch_area(u32 count, void* shapes) { return TotalAreaSwitch(count, (shape_union*)shapes); }
f32 switch_area4(u32 count, void* shapes) { return TotalAreaSwitch4(count, (shape_union*)shapes); }
f32 switch_corner(u32 count, void* shapes) { return CornerAreaSwitch(count, (shape_union*)shapes); }
f32 switch_corner4(u32 count, void* shapes) { return CornerAreaSwitch4(count, (shape_union*)shapes); }

f32 table_area(u32 count, void* shapes) { return TotalAreaUnion(count, (shape_union*)shapes); }
f32 table_area4(u32 count, void* shapes) { return TotalAreaUnion4(count, (shape_union*)shapes); }
f32 table_corner(u32 count, void* shapes) { return CornerAreaUnion(count, (shape_union*)shapes); }
f32 table_corner4(u32 count, void* shapes) { return CornerAreaUnion4(count, (shape_union*)shapes); }

int main() {
    // Prepare shapes for all versions
    // Calculate maximum sizeof of all shape classes
    size_t shape_base_size = sizeof(shape_base);
    size_t square_size = sizeof(square);
    size_t rectangle_size = sizeof(rectangle);
    size_t triangle_size = sizeof(triangle);
    size_t circle_size = sizeof(circle);
    size_t shape_union_size = sizeof(shape_union);
    
    size_t max_size = std::max({shape_base_size, square_size, rectangle_size, 
                               triangle_size, circle_size, shape_union_size});
        
    std::vector<shape_base*> vtbl_shapes;
    std::vector<shape_union> flat_shapes;
    
    // Create buffer vector for placement new
    std::vector<char> buffer(N * max_size);
    char* buffer_ptr = buffer.data();
    
    // Fill the buffer with shapes via placement new
    for (u32 i = 0; i < N; ++i) {
        switch (i % 4) {
            case 0:
                new(buffer_ptr) square(3.0f);
                break;
            case 1:
                new(buffer_ptr) rectangle(3.0f, 4.0f);
                break;
            case 2:
                new(buffer_ptr) triangle(3.0f, 4.0f);
                break;
            case 3:
                new(buffer_ptr) circle(3.0f);
                break;
        }
        buffer_ptr += max_size;
    }
    
    AreaCollector area_collector;
    OptimizedCornerCollector optimized_collector; // New OptimizedCornerCollector instance

    for (u32 i = 0; i < N; ++i) {
        switch (i % 4) {
            case 0:
                vtbl_shapes.push_back(new square(3.0f));
                flat_shapes.push_back({Shape_Square, 3.0f, 3.0f});
                break;
            case 1:
                vtbl_shapes.push_back(new rectangle(3.0f, 4.0f));
                flat_shapes.push_back({Shape_Rectangle, 3.0f, 4.0f});
                break;
            case 2:
                vtbl_shapes.push_back(new triangle(3.0f, 4.0f));
                flat_shapes.push_back({Shape_Triangle, 3.0f, 4.0f});
                break;
            case 3:
                vtbl_shapes.push_back(new circle(3.0f));
                flat_shapes.push_back({Shape_Circle, 3.0f, 3.0f});
                break;
        }
        area_collector.addShape(vtbl_shapes.back());
        optimized_collector.addShape(vtbl_shapes.back());
    }
    shape_base** vtbl_ptrs = vtbl_shapes.data();
    shape_union* flat_ptrs = flat_shapes.data();

    std::cout << "Benchmarking with " << N << " shapes..." << std::endl;
    
    std::cout << "=== Clean Code ===" << std::endl;
    bench("TotalArea", vtbl_area, N, vtbl_ptrs);
    bench("TotalArea4", vtbl_area4, N, vtbl_ptrs);
    bench("CornerArea", vtbl_corner, N, vtbl_ptrs);
    bench("CornerArea4", vtbl_corner4, N, vtbl_ptrs);

    std::cout << "=== Clean Code with Collectors ===" << std::endl;
    bench_total_collector("TotalAreaCollector", TotalAreaCollector, area_collector);
    bench_optimized_corner_collector("OptimizedCornerCollector", CornerAreaCollector, optimized_collector);

    std::cout << "=== Switch statement ===" << std::endl;
    bench("Switch TotalArea", switch_area, N, flat_ptrs);
    bench("Switch TotalArea4", switch_area4, N, flat_ptrs);
    bench("Switch CornerArea", switch_corner, N, flat_ptrs);
    bench("Switch CornerArea4", switch_corner4, N, flat_ptrs);

    std::cout << "=== Table-driven ===" << std::endl;
    bench("Table TotalArea", table_area, N, flat_ptrs);
    bench("Table TotalArea4", table_area4, N, flat_ptrs);
    bench("Table CornerArea", table_corner, N, flat_ptrs);
    bench("Table CornerArea4", table_corner4, N, flat_ptrs);

    // Cleanup
    for (auto ptr : vtbl_shapes) delete ptr;
    // local_shapes objects are in the buffer, will be destroyed when buffer goes out of scope
    return 0;
}
