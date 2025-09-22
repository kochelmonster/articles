#pragma once
#include <cstdint>
#include <cmath>
#include <vector>

using f32 = float;
using u32 = uint32_t;
constexpr f32 Pi32 = 3.14159265359f;

// Enum for switch/table versions
enum shape_type : u32 {
    Shape_Square,
    Shape_Rectangle,
    Shape_Triangle,
    Shape_Circle,
    Shape_Count
};

// Flat struct for switch/table versions
struct shape_union {
    shape_type Type;
    f32 Width;
    f32 Height;
};

// Base class for OOP version
class shape_base {
public:
    shape_base() { }
    virtual ~shape_base() {}
    virtual f32 Area() { return 0.0f; };
    virtual u32 CornerCount() { return 0; };
};

class square : public shape_base {
public:
    square(f32 SideInit) : Side(SideInit) {  }
    f32 Area() override { return Side * Side; }
    u32 CornerCount() override { return 4; }
    
private:
    f32 Side;
};

class rectangle : public shape_base {
public:
    rectangle(f32 WidthInit, f32 HeightInit) : Width(WidthInit), Height(HeightInit) {  }
    f32 Area() override { return Width * Height; }
    u32 CornerCount() override { return 4; }
private:
    f32 Width, Height;
};

class triangle : public shape_base {
public:
    triangle(f32 BaseInit, f32 HeightInit) : Base(BaseInit), Height(HeightInit) {  }
    f32 Area() override { return 0.5f * Base * Height; }
    u32 CornerCount() override { return 3; }
private:
    f32 Base, Height;
};

class circle : public shape_base {
public:
    circle(f32 RadiusInit) : Radius(RadiusInit) {  }
    f32 Area() override { return Pi32 * Radius * Radius; }
    u32 CornerCount() override { return 0; }
private:
    f32 Radius;
};

class AreaCollector {
public:
    AreaCollector() { }
    shape_base* addShape(shape_base* shape) {
        areas.push_back(shape->Area());
        return shape;
    }

    std::vector<f32> areas;
};

// Collector that precomputes weights
class CornerCollector {
public:
    CornerCollector() { }
    shape_base* addShape(shape_base* shape) {
        f32 area = shape->Area();
        u32 corner_count = shape->CornerCount();
        f32 weight = 1.0f / (1.0f + static_cast<f32>(corner_count));
        
        // Store area and precomputed weight
        areas.push_back(area);
        weights.push_back(weight);
        
        return shape;
    }

    std::vector<f32> areas;
    std::vector<f32> weights;
};
