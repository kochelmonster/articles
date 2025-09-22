#include "shapes.h"


// OOP area sum
f32 TotalAreaVTBL(u32 ShapeCount, shape_base **Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += Shapes[i]->Area();
    }
    return Accum;
}

f32 TotalAreaVTBL4(u32 ShapeCount, shape_base **Shapes) {
    f32 Accum0 = 0.0f, Accum1 = 0.0f, Accum2 = 0.0f, Accum3 = 0.0f;
    u32 Count = ShapeCount / 4;
    while (Count--) {
        Accum0 += Shapes[0]->Area();
        Accum1 += Shapes[1]->Area();
        Accum2 += Shapes[2]->Area();
        Accum3 += Shapes[3]->Area();
        Shapes += 4;
    }
    return Accum0 + Accum1 + Accum2 + Accum3;
}

// OOP corner-weighted area sum
f32 CornerAreaVTBL(u32 ShapeCount, shape_base **Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += (1.0f / (1.0f + (f32)Shapes[i]->CornerCount())) * Shapes[i]->Area();
    }
    return Accum;
}

f32 CornerAreaVTBL4(u32 ShapeCount, shape_base **Shapes) {
    f32 Accum0 = 0.0f, Accum1 = 0.0f, Accum2 = 0.0f, Accum3 = 0.0f;
    u32 Count = ShapeCount / 4;
    while (Count--) {
        Accum0 += (1.0f / (1.0f + (f32)Shapes[0]->CornerCount())) * Shapes[0]->Area();
        Accum1 += (1.0f / (1.0f + (f32)Shapes[1]->CornerCount())) * Shapes[1]->Area();
        Accum2 += (1.0f / (1.0f + (f32)Shapes[2]->CornerCount())) * Shapes[2]->Area();
        Accum3 += (1.0f / (1.0f + (f32)Shapes[3]->CornerCount())) * Shapes[3]->Area();
        Shapes += 4;
    }
    return Accum0 + Accum1 + Accum2 + Accum3;
}
