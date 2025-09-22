#include "shapes.h"

f32 GetAreaSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        case Shape_Square: return Shape.Width * Shape.Width;
        case Shape_Rectangle: return Shape.Width * Shape.Height;
        case Shape_Triangle: return 0.5f * Shape.Width * Shape.Height;
        case Shape_Circle: return Pi32 * Shape.Width * Shape.Width;
        default: return 0.0f;
    }
}

u32 GetCornerCountSwitch(shape_type Type) {
    switch (Type) {
        case Shape_Square: return 4;
        case Shape_Rectangle: return 4;
        case Shape_Triangle: return 3;
        case Shape_Circle: return 0;
        default: return 0;
    }
}

f32 TotalAreaSwitch(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += GetAreaSwitch(Shapes[i]);
    }
    return Accum;
}

f32 TotalAreaSwitch4(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum0 = 0.0f, Accum1 = 0.0f, Accum2 = 0.0f, Accum3 = 0.0f;
    ShapeCount /= 4;
    while (ShapeCount--) {
        Accum0 += GetAreaSwitch(Shapes[0]);
        Accum1 += GetAreaSwitch(Shapes[1]);
        Accum2 += GetAreaSwitch(Shapes[2]);
        Accum3 += GetAreaSwitch(Shapes[3]);
        Shapes += 4;
    }
    return Accum0 + Accum1 + Accum2 + Accum3;
}

f32 CornerAreaSwitch(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += (1.0f / (1.0f + (f32)GetCornerCountSwitch(Shapes[i].Type))) * GetAreaSwitch(Shapes[i]);
    }
    return Accum;
}

f32 CornerAreaSwitch4(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum0 = 0.0f, Accum1 = 0.0f, Accum2 = 0.0f, Accum3 = 0.0f;
    ShapeCount /= 4;
    while (ShapeCount--) {
        Accum0 += (1.0f / (1.0f + (f32)GetCornerCountSwitch(Shapes[0].Type))) * GetAreaSwitch(Shapes[0]);
        Accum1 += (1.0f / (1.0f + (f32)GetCornerCountSwitch(Shapes[1].Type))) * GetAreaSwitch(Shapes[1]);
        Accum2 += (1.0f / (1.0f + (f32)GetCornerCountSwitch(Shapes[2].Type))) * GetAreaSwitch(Shapes[2]);
        Accum3 += (1.0f / (1.0f + (f32)GetCornerCountSwitch(Shapes[3].Type))) * GetAreaSwitch(Shapes[3]);
        Shapes += 4;
    }
    return Accum0 + Accum1 + Accum2 + Accum3;
}
