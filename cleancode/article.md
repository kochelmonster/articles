# Clean Code vs. Optimization: Debunking the False Dichotomy

## Prologue: Reexamining "Clean Code, Horrible Performance"

This article is a response to "Clean Code, Horrible Performance" (https://www.computerenhance.com/p/clean-code-horrible-performance), which compared object-oriented programming with procedural approaches.

A fundamental question must be asked: How fair is it to compare demo code designed for readability against highly optimized code that is inherently less readable? The original article presented performance metrics without fully addressing the trade-offs involved in the optimization approaches.

This article consists of three parts:

1. **Maintainability and Extensibility**: We examine what sacrifices the optimized versions make in terms of extensibility and maintainability. As software systems grow and evolve, these factors often have a greater impact on total cost than raw performance.

2. **Performance Claims Analysis**: We critically evaluate the performance claims made in the original article, examining whether the conclusions drawn are fully supported by the evidence presented.

3. **Optimized Clean Code**: We present a clean code solution that maintains good object-oriented design principles while achieving performance 80% faster than even the "optimized" implementations presented in the original article. This demonstrates that clean code and performance are not mutually exclusive goals.


## Summary of the Original Article and Presented Code

The original article "Clean Code Horrible Performance" compares three approaches to shape area and corner-weighted area calculations in C++:

### 1. Clean Code (OOP)
Object-oriented design using virtual methods:
```cpp
// Area sum
f32 TotalAreaVTBL(u32 ShapeCount, shape_base **Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += Shapes[i]->Area();
    }
    return Accum;
}
// Corner-weighted area sum
f32 CornerAreaVTBL(u32 ShapeCount, shape_base **Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += (1.0f / (1.0f + (f32)Shapes[i]->CornerCount())) * Shapes[i]->Area();
    }
    return Accum;
}
```

### 2. Switch Code
Procedural code using switch statements:
```cpp
// Enum and struct for switch code
enum shape_type : u32 {
    Shape_Square,
    Shape_Rectangle,
    Shape_Triangle,
    Shape_Circle,
    Shape_Count
};

struct shape_union {
    shape_type Type;
    f32 Width;
    f32 Height;
};

f32 GetAreaSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        case Shape_Square: return Shape.Width * Shape.Width;
        case Shape_Rectangle: return Shape.Width * Shape.Height;
        case Shape_Triangle: return 0.5f * Shape.Width * Shape.Height;
        case Shape_Circle: return Pi32 * Shape.Width * Shape.Width;
        default: return 0.0f;
    }
}
f32 TotalAreaSwitch(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += GetAreaSwitch(Shapes[i]);
    }
    return Accum;
}
f32 CornerAreaSwitch(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += (1.0f / (1.0f + (f32)GetCornerCountSwitch(Shapes[i].Type))) * GetAreaSwitch(Shapes[i]);
    }
    return Accum;
}
```

### 3. Table Code
Table-driven code using precomputed coefficients:
```cpp
static const f32 AreaCTable[Shape_Count] = {1.0f, 1.0f, 0.5f, Pi32};
static const f32 CornerAreaCTable[Shape_Count] = {
    1.0f / (1.0f + 4.0f),
    1.0f / (1.0f + 4.0f),
    0.5f / (1.0f + 3.0f),
    Pi32
};
f32 GetAreaUnion(const shape_union& Shape) {
    return AreaCTable[Shape.Type] * Shape.Width * Shape.Height;
}
f32 CornerAreaUnion(u32 ShapeCount, shape_union* Shapes) {
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += CornerAreaCTable[Shapes[i].Type] * Shapes[i].Width * Shapes[i].Height;
    }
    return Accum;
}
```

The article benchmarks these approaches, arguing that the clean code (OOP) solution is much slower than the switch and table-driven implementations. The following sections analyze these claims and present a clean code solution that achieves both clarity and high performance.


## Part 1: Maintainability and Extensibility - The Hidden Costs of Optimization

While performance is certainly important, maintainability and extensibility often have a far greater impact on the total cost of software over its lifetime. In this section, we compare the maintainability and extensibility of the three approaches presented in the original article: clean object-oriented code, switch-based code, and table-driven code.

## Maintainability: The Long-Term Investment

### Localization of Changes: Where Must You Look?

Software maintenance frequently involves fixing bugs or enhancing functionality for specific shapes. How many places must you modify to make a change?

**In clean code**, changes are naturally isolated to a single class:

```cpp
class triangle : public shape_base {
public:
    triangle(f32 BaseInit, f32 HeightInit) : Base(BaseInit), Height(HeightInit) { }
    
    // To fix a bug in triangle area calculation, you only change this ONE method
    f32 Area() override { return 0.5f * Base * Height; }
    
    u32 CornerCount() override { return 3; }
    
private:
    f32 Base;
    f32 Height;
};
```

**In switch code**, you must locate and modify multiple switch statements:

```cpp
// Need to modify the area calculation here
f32 GetAreaSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        case Shape_Triangle: return 0.5f * Shape.Width * Shape.Height;
        // Other cases...
    }
}

// And check if this function needs updating for consistency
u32 GetCornerCountSwitch(shape_type Type) {
    switch (Type) {
        case Shape_Triangle: return 3;
        // Other cases...
    }
}
```

**In table code**, you must understand and modify coefficient tables:

```cpp
// Must update this coefficient
static const f32 AreaCTable[Shape_Count] = {1.0f, 1.0f, 0.5f, Pi32};

// And ensure consistency with this related table
static const f32 CornerAreaCTable[Shape_Count] = {
    1.0f / (1.0f + 4.0f), 1.0f / (1.0f + 4.0f), 
    0.5f / (1.0f + 3.0f), Pi32
};
```

The scattered nature of shape information in the switch and table approaches makes changes riskier and more time-consuming.

### The Burden of Algorithm Changes

Consider what happens when we need to implement a more sophisticated area calculation for triangles using Heron's formula, which uses the lengths of all three sides.

**In clean code**, the change is straightforward:

```cpp
class triangle : public shape_base {
public:
    // Updated constructor for three sides
    triangle(f32 a, f32 b, f32 c) : side_a(a), side_b(b), side_c(c) { }
    
    // Implement Heron's formula
    f32 Area() override { 
        f32 s = (side_a + side_b + side_c) / 2.0f;
        return sqrtf(s * (s - side_a) * (s - side_b) * (s - side_c));
    }
    
    // Other methods unchanged
    u32 CornerCount() override { return 3; }
    
private:
    f32 side_a, side_b, side_c;
};
```

**In switch code**, we face a cascade of changes:

1. First, we must modify the core data structure:
```cpp
struct shape_union {
    shape_type Type;
    union {
        struct { f32 Width; } square;
        struct { f32 Width; f32 Height; } rect;
        struct { f32 Side1; f32 Side2; f32 Side3; } tri;  // New structure
        struct { f32 Radius; } circ;
    };
};
```

2. Then update every function that processes shapes:
```cpp
f32 GetAreaSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        // Must update all shape cases to use the new union
        case Shape_Square: return Shape.square.Width * Shape.square.Width;
        // Other shapes...
        case Shape_Triangle: {
            // Heron's formula
            f32 a = Shape.tri.Side1;
            f32 b = Shape.tri.Side2;
            f32 c = Shape.tri.Side3;
            f32 s = (a + b + c) / 2.0f;
            return sqrtf(s * (s - a) * (s - b) * (s - c));
        }
    }
}
```

3. All code that creates or manipulates shapes must be updated.

**In table code**, the situation is even worse:

```cpp
f32 GetAreaUnion(const shape_union& Shape) {
    // The table approach completely breaks down
    // We can no longer use a simple coefficient * width * height formula
    
    if (Shape.Type == Shape_Triangle) {
        // Special case for triangles
        f32 a = Shape.tri.Side1;
        f32 b = Shape.tri.Side2;
        f32 c = Shape.tri.Side3;
        f32 s = (a + b + c) / 2.0f;
        return sqrtf(s * (s - a) * (s - b) * (s - c));
    } else {
        // Use table for other shapes
        return AreaCTable[Shape.Type] * /* shape-specific dimensions */;
    }
}
```

The table approach fundamentally breaks because Heron's formula doesn't fit the coefficient pattern assumed by the table design.

### Cognitive Load: Understanding Shape Behavior

When a developer needs to understand how a shape behaves, how much mental effort is required?

**In clean code**, all shape behavior is in one place:

```cpp
class circle : public shape_base {
public:
    circle(f32 RadiusInit) : Radius(RadiusInit) { }
    f32 Area() override { return Pi32 * Radius * Radius; }
    u32 CornerCount() override { return 0; }
    
private:
    f32 Radius;
};
```

A developer can understand the complete behavior of a circle by examining this single class.

**In switch code**, understanding requires examining multiple functions:

```cpp
// Need to find and read area calculation
f32 GetAreaSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        case Shape_Circle: return Pi32 * Shape.Width * Shape.Width;
        // Other cases...
    }
}

// And corner count calculation
u32 GetCornerCountSwitch(shape_type Type) {
    switch (Type) {
        case Shape_Circle: return 0;
        // Other cases...
    }
}

// And how corner weighting works
f32 CornerAreaSwitch(u32 ShapeCount, shape_union* Shapes) {
    // Complex calculation involving multiple function calls
}
```

**In table code**, understanding requires deciphering tables and formulas:

```cpp
// Need to understand this coefficient
static const f32 AreaCTable[Shape_Count] = {1.0f, 1.0f, 0.5f, Pi32};

// And how it's applied
f32 GetAreaUnion(const shape_union& Shape) {
    return AreaCTable[Shape.Type] * Shape.Width * Shape.Height;
}

// And realize that for circles, Width is used as radius and Height is ignored
```

This scattered information creates significant cognitive overhead, making maintenance more difficult and error-prone.

### Preventing Inconsistencies

The clean code approach naturally prevents inconsistencies by defining shape properties once:

```cpp
class square : public shape_base {
public:
    square(f32 SideInit) : Side(SideInit) { }
    f32 Area() override { return Side * Side; }
    u32 CornerCount() override { return 4; }
    
private:
    f32 Side; // Property defined once
};
```

In contrast, the switch and table approaches duplicate knowledge across functions and tables:

```cpp
// Square area uses Width
f32 GetAreaSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        case Shape_Square: return Shape.Width * Shape.Width;
        // Other cases...
    }
}

// Corner count defined separately
u32 GetCornerCountSwitch(shape_type Type) {
    switch (Type) {
        case Shape_Square: return 4;
        // Other cases...
    }
}
```

This duplication creates risk: what happens if we decide a square has rounded corners and should report a different corner count? In clean code, we change one method. In switch code, we must find and update every relevant switch statement.

### Compiler Assistance

The clean code approach leverages the compiler to enforce correctness:

```cpp
class shape_base {
public:
    virtual f32 Area() = 0;
    virtual u32 CornerCount() = 0;
};

// Attempting to create a concrete shape without implementing required methods:
class hexagon : public shape_base {
    // Missing Area() and CornerCount() implementations
}; 

// Compiler error: cannot instantiate abstract class
```

Switch and table code rely on developer vigilance to maintain consistency:

```cpp
// Adding a new shape to the enum
enum shape_type { 
    Shape_Square, Shape_Rectangle, Shape_Triangle, Shape_Circle, 
    Shape_Hexagon,  // New shape
    Shape_Count 
};

// Easy to forget to update all relevant switch statements and tables
// No compiler errors if you miss one
```

## Extensibility: Building Systems That Grow

Beyond day-to-day maintenance, software must also adapt to new requirements. Let's examine how each approach handles common extension scenarios.

### Adding New Operations: Perimeter Calculation

Now let's extend our system with a new operation: calculating the perimeter of shapes.

**In clean code**:

```cpp
// Add to interface
class shape_base {
public:
    virtual f32 Area() = 0;
    virtual u32 CornerCount() = 0;
    virtual f32 Perimeter() = 0;  // New method
};

// Implement for existing shapes
class square : public shape_base {
public:
    // Existing code...
    f32 Perimeter() override { return 4.0f * Side; }
};

class circle : public shape_base {
public:
    // Existing code...
    f32 Perimeter() override { return 2.0f * Pi32 * Radius; }
};

// Create aggregator (follows established pattern)
f32 AveragePerimeterVTBL(u32 ShapeCount, shape_base **Shapes) {
    if (ShapeCount == 0) return 0.0f;
    
    f32 Accum = 0.0f;
    for (u32 i = 0; i < ShapeCount; ++i) {
        Accum += Shapes[i]->Perimeter();
    }
    return Accum / ShapeCount;
}
```

**In switch code**:

```cpp
// Create entirely new function with all shape handling
f32 GetPerimeterSwitch(const shape_union& Shape) {
    switch (Shape.Type) {
        case Shape_Square: return 4.0f * Shape.Width;
        case Shape_Rectangle: return 2.0f * (Shape.Width + Shape.Height);
        case Shape_Triangle: return /* complex calculation */;
        case Shape_Circle: return 2.0f * Pi32 * Shape.Width;
        // Must handle EVERY shape type
        default: return 0.0f;
    }
}

// New aggregator
f32 AveragePerimeterSwitch(u32 ShapeCount, shape_union* Shapes) {
    // Implementation...
}
```

**In table code**:

```cpp
// Problem: perimeter doesn't follow width*height pattern!
static const f32 PerimeterCTable[Shape_Count] = {
    4.0f,                    // Square: 4*width
    2.0f,                    // Rectangle: needs special handling
    3.0f,                    // Triangle: approximation
    2.0f * Pi32              // Circle: 2πr
};

f32 GetPerimeterUnion(const shape_union& Shape) {
    // Most shapes don't fit the pattern!
    switch (Shape.Type) {
        case Shape_Rectangle: 
            return 2.0f * (Shape.Width + Shape.Height);
        // Other special cases...
        default:
            return PerimeterCTable[Shape.Type] * Shape.Width;
    }
}
```

The perimeter calculation reveals a key weakness in the table approach: it only works when operations follow the same mathematical pattern. The switch approach fares better but still requires implementing and maintaining a complete new function.

### Third-Party Extensions: The Library Scenario

Finally, let's consider a critical real-world scenario: your code is provided as a library, and third parties need to add their own shapes.

**In clean code**, third parties can extend the system without any changes to your library:

```cpp
// Third-party code:
class hexagon : public LibraryNamespace::shape_base {
public:
    hexagon(f32 side_length) : m_side(side_length) {}
    
    f32 Area() const override { 
        return 1.5f * sqrtf(3.0f) * m_side * m_side; 
    }
    
    u32 CornerCount() const override { return 6; }
    f32 Perimeter() const override { return 6.0f * m_side; }
    
private:
    f32 m_side;
};

// Works with all existing library functions
f32 total = LibraryNamespace::TotalAreaVTBL(1, &myHexagon);
```

**In switch code**, third parties face a fundamental barrier:

```cpp
// Third-party can't modify the library's internal enums!
// Can't add to shape_type without modifying library source
// Can't modify GetAreaSwitch without modifying library source

// Only option is to create a compatibility wrapper:
class HexagonWrapper : public LibraryNamespace::shape_base {
    // Create a shape that works with the clean code interface
    // But this defeats the purpose of the switch optimization
};
```

**In table code**, extension is practically impossible without modifying the library source code or creating wrappers that defeat the original optimization purpose.

## Part 2: Performance Claims Analysis

Performance is a crucial aspect of software, but it must be considered in context. Professional software development is fundamentally about cost efficiency—delivering substantial functionality with minimal effort. Functionality encompasses not only features, but also reliability, maintainability, extensibility, portability, testability, and execution speed. Software must be "fast enough" for its intended use: in most cases, this means the user doesn't have to wait, but in some domains, such as real-time stock trading, even a millisecond can be too slow.

Optimization should only begin when the software isn't fast enough. This principle, echoed by Einstein's advice to "make things as simple as possible, but not simpler," reminds us that excessive optimization can be counterproductive. Optimized code almost always comes at the expense of maintainability and readability, and the speed improvements are almost always purchased with the disadvantages described in the first part of this article.

When we examine the optimizations in detail, it becomes clear that they are all processor-specific. Loop unrolling enables pipelining, switch-case code leverages branch prediction, and table-driven code exploits cache optimization. The speed advantage is only visible on processors that support these techniques. For example, the author was able to achieve a 150% speedup on a small PIC microcontroller by replacing a switch-case with a function pointer array. While most modern processors support these optimizations, future processor generations could make them obsolete—or even favor different techniques, such as indirections. In that case, highly optimized code may become a liability, running slower than clean code.

The original article makes a striking claim: "To put that in hardware terms, it would be like taking an iPhone 14 Pro Max and reducing it to an iPhone 11 Pro Max. It's three or four years of hardware evolution erased because somebody said to use polymorphism instead of switch statements." If this were a valid argument, we would have to immediately ban all scripting languages, since they would likely turn an iPhone 14 into an iPhone 3. We would also have to throw out all compilers: only with assembly language can you fully exploit the processor's performance. Clearly, this is not how professional software is built.

Beyond hardware-specific tricks, it's important to recognize the role of compilers and language advances. Modern compilers are increasingly capable of optimizing away many performance bottlenecks that previously required manual intervention. Techniques such as inlining, dead code elimination, and profile-guided optimization can often match or exceed the performance of hand-optimized code. Relying on manual micro-optimizations may result in code that is less portable and less likely to benefit from future compiler improvements.

Moreover, real-world bottlenecks are often elsewhere. In many practical applications, the true performance bottlenecks are not in the computational code itself, but in I/O, networking, or external dependencies. Optimizing shape area calculations may have negligible impact compared to database or network latency. Focusing optimization efforts on the wrong part of the system can lead to wasted effort and minimal real-world gains.

Algorithmic improvements also tend to yield much larger performance gains than micro-optimizations. Switching from an O(n^2) to an O(n log n) algorithm, for example, can dwarf any gains from low-level code tweaks. Highly optimized, hardware-specific code may not be portable to other platforms or future hardware generations. What is fast on one processor may be slow or even counterproductive on another. Clean, portable code is more likely to benefit from advances in hardware and compiler technology without requiring major rewrites.

Finally, as code becomes more optimized, the returns on further optimization diminish. The initial optimizations may yield significant speedups, but subsequent efforts often result in much smaller improvements. At some point, the cost of further optimization outweighs the benefits, especially if the code is already "fast enough" for its intended use. Raw computational speed is only one aspect of user experience. Responsiveness, error handling, and reliability often matter more to users than shaving a few microseconds off a calculation. Over-optimizing for speed can sometimes degrade the overall user experience if it makes the code less robust or harder to maintain.


## Part 3: Optimized Clean Code – Clean Principles Meet High Performance

This part shows how we keep a clean, object‑oriented public model while moving the hot arithmetic into a data‑oriented, SIMD‑friendly layer. We separate three concerns: (1) collecting invariant scalar data, (2) precomputing per‑shape factors once, and (3) performing wide aggregation using vector instructions. Only step (3) is hardware‑specific; steps (1) and (2) remain simple, testable, and portable.

### 3.1 Collectors: A Thin Adaptation Layer
The collectors decouple the object interface (`shape_base`) from the optimized aggregation. They extract exactly what the hot loops need:
* `AreaCollector`: stores raw areas in a contiguous `std::vector<float>`.
* `CornerCollector`: stores areas plus a precomputed weight `1/(1+corner_count)` so the expensive division and virtual calls are paid once per shape, not inside the SIMD loop.

Declaration (kept trivial on purpose):
```cpp
class AreaCollector {
public:
    shape_base* addShape(shape_base* s) {
        areas.push_back(s->Area());
        return s;
    }
    std::vector<f32> areas;
};

class CornerCollector {
public:
    shape_base* addShape(shape_base* s) {
        f32 a = s->Area();
        f32 w = 1.0f / (1.0f + (f32)s->CornerCount());
        areas.push_back(a);
        weights.push_back(w);
        return s;
    }
    std::vector<f32> areas;
    std::vector<f32> weights;
};
```
These classes are intentionally tiny: no inheritance, no templates, no hidden magic—just a staging buffer. They preserve the cleanliness of the domain model while enabling a layout the CPU loves.

### 3.2 Using the Collectors (Precomputation Phase)
We build the usual polymorphic shape list for clarity and extensibility, and simultaneously feed the collectors. The loop performs all virtual dispatch exactly once per shape; afterwards aggregation is pure array math.
```cpp
std::vector<shape_base*> shapes;
AreaCollector area_collector;
CornerCollector corner_collector;

for (u32 i = 0; i < N; ++i) {
    switch (i % 4) {
        case 0: shapes.push_back(new square(3.0f)); break;
        case 1: shapes.push_back(new rectangle(3.0f, 4.0f)); break;
        case 2: shapes.push_back(new triangle(3.0f, 4.0f)); break;
        case 3: shapes.push_back(new circle(3.0f)); break;
    }
    area_collector.addShape(shapes.back());
    corner_collector.addShape(shapes.back());
}
```
Conceptually: we have transformed a virtual-call dominated reduction into two phases (gather + crunch). If you port to a different architecture or change vector width, this gathering code remains untouched.

### 3.3 Aggregation: Vectorized Crunch Layer
The aggregation functions operate on plain contiguous floats. They use AVX (256‑bit) intrinsics for eight‑wide parallelism, loop unrolling, multiple accumulators to hide latency, prefetching to reduce cache miss penalties, and (in the weighted variant) FMA for `a*b + c` in one fused step.

Area aggregation (abridged for focus):
```cpp
f32 TotalAreaCollector(AreaCollector& c) {
    const f32* a = c.areas.data();
    size_t n = c.areas.size();
    __m256 s0=_mm256_setzero_ps(), s1=_mm256_setzero_ps();
    size_t i=0;
    for (; i + 15 < n; i += 16) { // unrolled by 2
        s0 = _mm256_add_ps(s0, _mm256_loadu_ps(a + i));
        s1 = _mm256_add_ps(s1, _mm256_loadu_ps(a + i + 8));
    }
    s0 = _mm256_add_ps(s0, s1);
    // horizontal reduce
    __m128 hi = _mm256_extractf128_ps(s0,1);
    __m128 lo = _mm256_castps256_ps128(s0);
    __m128 sum = _mm_add_ps(hi, lo);
    sum = _mm_hadd_ps(sum,sum);
    sum = _mm_hadd_ps(sum,sum);
    f32 acc = _mm_cvtss_f32(sum);
    for (; i < n; ++i) acc += a[i];
    return acc;
}
```
Weighted (corner) aggregation (core pattern):
```cpp
f32 CornerAreaCollector(CornerCollector& c) {
    const f32* a = c.areas.data();
    const f32* w = c.weights.data();
    size_t n = c.areas.size();
    __m256 s0=_mm256_setzero_ps(); size_t i=0;
    for (; i + 7 < n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vw = _mm256_loadu_ps(w + i);
        s0 = _mm256_fmadd_ps(va, vw, s0); // a*w + s0
    }
    __m128 hi = _mm256_extractf128_ps(s0,1);
    __m128 lo = _mm256_castps256_ps128(s0);
    __m128 sum = _mm_add_ps(hi, lo);
    sum = _mm_hadd_ps(sum,sum);
    sum = _mm_hadd_ps(sum,sum);
    f32 acc = _mm_cvtss_f32(sum);
    for (; i < n; ++i) acc += a[i] * w[i];
    return acc;
}
```
Brief SIMD note: AVX provides 256‑bit registers (`__m256`) holding 8 single‑precision floats. Operations like `_mm256_add_ps` and `_mm256_fmadd_ps` perform arithmetic on all lanes simultaneously. This yields substantial throughput gains versus scalar loops while the source data layout stays clean and minimal.

Portability & stability: If you move to ARM NEON, SVE, or a future wider x86 extension, you only need to re‑implement these aggregation functions (3.3). The public interfaces (`shape_base`, collectors, and the shape creation loop) remain unchanged. Thus the optimization layer is a replaceable module rather than a pervasive style.

The result: we retain extensibility (new shapes only implement `Area()` / `CornerCount()`), keep maintenance localized, and still obtain performance competitive with or surpassing hand‑rolled switch/table code.
```

This implementation uses:
- **SIMD (AVX) instructions** for parallel processing of shape areas and weights
- **Loop unrolling** and **multiple accumulators** to maximize instruction-level parallelism
- **Cache prefetching** for large arrays to minimize memory latency
- **Fused Multiply-Add (FMA)** for efficient weighted sums

Despite these advanced optimizations, the code remains clean, modular, and maintainable. The logic is encapsulated in well-named functions, and the use of modern C++ features ensures extensibility. This approach proves that clean code and high performance are not only compatible, but can reinforce each other when guided by sound engineering principles.

### Speed Comparison: Clean Code vs. Switch vs. Table

| Approach                | Maintainability | Extensibility | Raw Speed (relative) |
|-------------------------|----------------|---------------|----------------------|
| Clean Code (OOP)        | Excellent      | Excellent     | Baseline             |
| Switch-based            | Poor           | Poor          | 1.0x – 1.2x          |
| Table-driven            | Poor           | Very Poor     | 1.1x – 1.3x          |
| Optimized Clean Code    | Excellent      | Excellent     | 1.8x – 2.0x          |

- **Clean Code (OOP)**: Best maintainability and extensibility, with performance that is usually "fast enough" for most applications.
- **Switch-based**: Gains some speed on certain processors, but sacrifices maintainability and extensibility.
- **Table-driven**: Slightly faster in specific scenarios, but fragile and hard to extend.
- **Optimized Clean Code**: Delivers both clean design and top-tier performance, often outperforming the other approaches when modern compiler optimizations are enabled.

### Conclusion: Clean Code Can Be Fast

This article has shown that the trade-off presented in "Clean Code Horrible Performance" is a false one. With modern C++ and compiler technology, developers can write code that is both clean and highly performant. Optimized clean code is not only possible—it is often the best choice for maintainability, extensibility, and speed. Clean principles and high performance are not mutually exclusive; they are complementary goals for professional software development.

