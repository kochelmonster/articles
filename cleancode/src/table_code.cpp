#include "shapes.h"

// Table-driven coefficients for area and corner-weighted area
static const f32 AreaCTable[Shape_Count] = {1.0f, 1.0f, 0.5f, Pi32};
static const f32 CornerAreaCTable[Shape_Count] = {
    1.0f / (1.0f + 4.0f), // Square
    1.0f / (1.0f + 4.0f), // Rectangle
    0.5f / (1.0f + 3.0f), // Triangle
    Pi32 // Circle (corner count is 0, so just Pi32)
};

f32 GetAreaUnion(const shape_union& Shape) {
    return AreaCTable[Shape.Type] * Shape.Width * Shape.Height;
}

f32 GetCornerAreaUnion(const shape_union& Shape) {
    return CornerAreaCTable[Shape.Type] * Shape.Width * Shape.Height;
}

f32 TotalAreaUnion(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += GetAreaUnion(Shapes[i]);
    }
    return Accum;
}

f32 TotalAreaUnion4(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum0 = 0.0f, Accum1 = 0.0f, Accum2 = 0.0f, Accum3 = 0.0f;
    ShapeCount /= 4;
    while (ShapeCount--) {
        Accum0 += GetAreaUnion(Shapes[0]);
        Accum1 += GetAreaUnion(Shapes[1]);
        Accum2 += GetAreaUnion(Shapes[2]);
        Accum3 += GetAreaUnion(Shapes[3]);
        Shapes += 4;
    }
    return Accum0 + Accum1 + Accum2 + Accum3;
}

f32 CornerAreaUnion(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += GetCornerAreaUnion(Shapes[i]);
    }
    return Accum;
}

f32 CornerAreaUnion4(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum0 = 0.0f, Accum1 = 0.0f, Accum2 = 0.0f, Accum3 = 0.0f;
    ShapeCount /= 4;
    while (ShapeCount--) {
        Accum0 += GetCornerAreaUnion(Shapes[0]);
        Accum1 += GetCornerAreaUnion(Shapes[1]);
        Accum2 += GetCornerAreaUnion(Shapes[2]);
        Accum3 += GetCornerAreaUnion(Shapes[3]);
        Shapes += 4;
    }
    return Accum0 + Accum1 + Accum2 + Accum3;
}
