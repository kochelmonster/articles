# Clean Code vs. Optimization: Debunking the False Dichotomy

## Prologue: Reexamining "Clean Code, Horrible Performance"

This article is a response to "Clean Code, Horrible Performance" ([https://www.computerenhance.com/p/clean-code-horrible-performance](https://www.computerenhance.com/p/clean-code-horrible-performance)) of Casey Muratori. He argues that the principles of "clean code" as popularized by Robert C. Martin (Uncle Bob) lead to poor performance in software, using shape area calculations as a case study. The "clean code" methodology would make software dramatically slower with four out of five core principles of clean coding being detrimental to performance, resulting in code that can be 15 times slower or worse. According to Casey Muratori, sacrificing a decade of hardware performance improvements just for programmer convenience is unacceptable, and that while code organization and maintainability are worthy goals, the current "clean code" rules fail to achieve them without severe performance penalties.

This answer article consists of 5 parts:

1. A short Introduction to the code Casey Muratori presented.

2. A general Discussion about the goals of professional software development and the ranking of runtime performance against other quality attributes.

3. I examine what sacrifices Casey had to make with his optimized versions make in terms of extensibility and maintainability. 

4. I critically evaluate the performance claims of Casey, examining whether the conclusions drawn are fully supported by the evidence presented.

5. I present a clean code solution that maintains good object-oriented design principles while achieving performance more than 300% faster than even the "optimized" implementations presented in the original article. This demonstrates that clean code and performance are not mutually exclusive goals.

## Part 1: The Code Competing for Performance

The original article "Clean Code Horrible Performance" compares three approaches to shape area and corner-weighted area calculations in C++:

### Clean Code (OOP)

A clean object-oriented design using virtual methods:

```cpp
// Base class and derived shape classes
class shape_base {
public:
    virtual f32 Area() = 0;
    virtual u32 CornerCount() = 0;
};

class square : public shape_base {
public:
    square(f32 SideInit) : Side(SideInit) { }
    f32 Area() override { return Side * Side; }
    u32 CornerCount() override { return 4; }
private:
    f32 Side;
};

class rectangle : public shape_base {
public:
    rectangle(f32 WidthInit, f32 HeightInit) : Width(WidthInit), Height(HeightInit) { }
    f32 Area() override { return Width * Height; }
    u32 CornerCount() override { return 4; }
private:
    f32 Width, Height;
};

class triangle : public shape_base {
public:
    triangle(f32 BaseInit, f32 HeightInit) : Base(BaseInit), Height(HeightInit) { }
    f32 Area() override { return 0.5f * Base * Height; }
    u32 CornerCount() override { return 3; }
private:
    f32 Base, Height;
};

class circle : public shape_base {
public:
    circle(f32 RadiusInit) : Radius(RadiusInit) { }
    f32 Area() override { return Pi32 * Radius * Radius; }
    u32 CornerCount() override { return 0; }
private:
    f32 Radius;
};

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

The clean code approach uses classic object-oriented principles. Each shape is encapsulated in its own class with specific behavior and data. The abstract `shape_base` class defines a common interface through pure virtual methods. At runtime, virtual function calls dispatch through vtables to the appropriate implementation, enabling polymorphism—the same aggregation code works with any mix of shape types.


### Switch Code

Procedural code using switch statements:

```cpp
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

The switch approach abandons object-oriented design in favor of procedural programming. All shapes are represented by a single `shape_union` struct containing a type enum and generic dimensions (`Width`, `Height`). Shape-specific behavior is implemented through switch statements that branch based on the `Type` field. This eliminates virtual function calls and vtable lookups, allowing the compiler to generate jump tables or use branch prediction more effectively.

### Table Code

Table-driven code using precomputed coefficients:

```cpp
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

The table-driven approach further simplifies the switch method by replacing conditional logic with precomputed tables of coefficients. Each shape type has associated constants for area calculation and corner-weighting stored in arrays. The area and corner-area functions simply index into these tables based on the shape type, multiplying the coefficients by the generic dimensions. This minimizes branching and can improve cache locality, potentially leading to better performance.


## Part 2: Software Quality Attributes

Object-orientation and clean coding was not developed as a new fancy way of writing code: It is an answer of problems the developers community faced during the life cycle of software projects, especially Maintainability and Extensibility.

In the 1990's I worked for several banks and learned an unforeseen problem could lead to massive costs:
The programmers of the boot routines for their main frames died already and no one understood the functional mess anymore. So the banks invested millions of dollars for hardware to keep the main frames running: Everyone feared after a shutdown the whole system wouldn't boot up anymore.

Clean coding is not just about programmers "inconvenience", but about cost efficiency. Every optimization needs extra development time, extra testing code, and extra documentation. Often the the optimation use tricks that make the code less readable and harder to maintain. Is your customer ready to pay this extra costs for a speed improvment he does not even notice?

Runtime performance is a crucial aspect of software, but it must be considered in context. Professional software development is fundamentally about cost efficiency—delivering substantial functionality with minimal effort. With Functionality I mean not only features, but also reliability, maintainability, extensibility, portability, testability, and finally execution speed. Software must be "fast enough" for its intended use: in many cases, this just means the user doesn't have to wait, but in some domains, even a millisecond can be too slow.

Optimization should only begin when the software isn't fast enough. Freely adapted from Einstein's advice: "make software as fast as needed, but not faster."

## Part 3: Maintainability and Extensibility - The Hidden Costs of Optimization

Let's examine how Casey Muratori's code performans during a typical software lifecycle.

### The Burden of Algorithm Changes

What happens when we need to implement a more sophisticated area calculation for triangles using Heron's formula, which uses the lengths of all three sides?

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

1. First, we must modify the core data structure, because the existing `shape_union` is insufficient to represent a triangle with three sides:

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

Because Casey Muray changed the whole architecture for better runtime performance, a simple implementation change of one shape's method requires a complete rewrite of the whole program! In table code it even breaks the whole structure.


### The Library Scenario

As you see the optimized code does not so well in maintainability. How does it perform with extensibility? The original clean code examples shows a quite typical library scenario. And good libray should be easily extended by own shapes. Lets try it out and add a hexagon shape.

**In clean code**, third parties can extend the system without any changes to your library:

```cpp
// Third-party code:
class hexagon : public shape_base {
public:
    hexagon(f32 side_length) : m_side(side_length) {}
    
    f32 Area() const override { 
        return 1.5f * sqrtf(3.0f) * m_side * m_side; 
    }
    
    u32 CornerCount() const override { return 6; }
    
private:
    f32 m_side;
};

// Works with all existing library functions
f32 total = TotalAreaVTBL(1, &myHexagon);
```

**In switch code**, you have have to change every function to insert your hexagon code! If you want to sell your library to third parties, you have to provide the complete source code, so the can modify it. But what will happen, when you release the next version?

**In table code**, extension is practically impossible without modifying the library source code or creating wrappers that defeat the original optimization purpose.

Casey Muratori's optimizations come at a high cost: It is neither maintainable nor extensible. In what will your customer invest his money? 
Extensible and maintainable software that needs a faster hardware (which gets cheaper and faster every year) or a highly optimized but inflexible and unmaintainable software that can run on cpu's from 10 years ago?


## Part 4: Performance Claims Analysis

Unfortunately the Casey Muratori did not publish the benchmark code he used to measure the speed. According to his article his version run 25 times faster than the clean code version. I used his code samples and wrote my own benchmark code. Here is the result on a AMD Ryzen 7 8745H:

| Benchmark | Time (ms) | Speedup Factor |
|-----------|-----------|----------------|
| Clean Code TotalArea | 2.505 | 1.0x (baseline) |
| Clean Code TotalArea4 | 1.007 | 2.49x |
| Clean Code CornerArea | 5.355 | 1.0x (baseline) |
| Clean Code CornerArea4 | 2.153 | 2.49x |
| Switch TotalArea | 0.661 | 3.79x |
| Switch TotalArea4 | 0.397 | 6.31x |
| Switch CornerArea | 0.637 | 8.41x |
| Switch CornerArea4 | 0.536 | 9.99x |
| Table TotalArea | 0.633 | 3.96x |
| Table TotalArea4 | 0.334 | 7.50x |
| Table CornerArea | 0.626 | 8.56x |
| Table CornerArea4 | 0.332 | 16.13x |

So on my system the best optimized version is "only" 16 times faster than the clean code version. (Compared to the unrolled clean code version it is "only" 6.5 times faster.)

Thus is not surprising: all the optimizations using processor-specific features:

- Loop unrolling enables pipelining
- switch-case code leverages branch prediction
- table-driven code exploits cache optimization

The speed advantage is only visible on processors that support these techniques. While most modern processors support these optimizations, future processor generations could make them obsolete—or even favor different techniques, such as indirections. In that case, highly optimized code may become a liability, running slower than clean code.
On older or simpler cpus the optimizations can even be counterproductive: For example I once achieved a 150% speedup on a small PIC microcontroller by replacing a switch-case with a function pointer array.

Casey Muratori makes a striking claim: 

> *To put that in hardware terms, it would be like taking an iPhone 14 Pro Max and reducing it to an iPhone 11 Pro Max. It's three or four years of hardware evolution erased because somebody said to use polymorphism instead of switch statements.*

On one side it is a kind of circular reasoning: The optimization he uses only work on the modern hardware. On the other side if this were a valid argument, we would have to immediately ban all scripting languages, since they would likely turn an iPhone 14 into an iPhone 3. Consequently we should also have to throw out all compilers: only with assembler you can fully exploit the processor's performance. 

Beyond hardware-specific tricks, it's important to recognize the role of compilers and language advances. Modern compilers are increasingly capable of optimizing away many performance bottlenecks that previously required manual intervention. Techniques such as inlining, dead code elimination, and profile-guided optimization can often match or exceed the performance of hand-optimized code. Relying on manual micro-optimizations may result in code that is less portable and less likely to benefit from future compiler improvements.

Moreover, real-world bottlenecks are often elsewhere. In many practical applications, the true performance bottlenecks are not in the computational code itself, but in I/O, networking, or external dependencies. Optimizing shape area calculations may have negligible impact compared to database or network latency. Focusing optimization efforts on the wrong part of the system can lead to wasted effort and minimal real-world gains.

Algorithmic improvements also tend to yield much larger performance gains than micro-optimizations. Switching from an O(n²) to an O(n log n) algorithm, for example, can dwarf any gains from low-level code tweaks. Highly optimized, hardware-specific code may not be portable to other platforms or future hardware generations. What is fast on one processor may be slow or even counterproductive on another. Clean, portable code is more likely to benefit from advances in hardware and compiler technology without requiring major rewrites.


## Part 5: Optimized Clean Code – Clean Principles Meet High Performance

This part shows how we keep a clean, object‑oriented public model while moving the hot arithmetic into a data‑oriented, SIMD‑friendly layer. We separate three concerns: 

1. collecting invariant scalar data, 
2. precomputing per‑shape factors once, and 
3. performing wide aggregation using vector instructions. 
    
Only step 3 is hardware‑specific; steps 1 and 2 remain clean and object oriented.

### 5.1 Collectors: A Thin Adaptation Layer

The collectors decouple the object interface ( `shape_base` ) from the optimized aggregation. They extract exactly what the hot loops need:

* `AreaCollector`: stores raw areas in a contiguous `std::vector<float>`.

* `CornerCollector`: stores areas plus a precomputed weight `1/(1+corner_count)` so the expensive division and virtual calls are paid once per shape, not inside the SIMD loop.

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

These classes preserve the cleanliness of the domain model while enabling a layout the CPU loves.

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
f32 TotalAreaCollector(AreaCollector& collector) {
    // Use SIMD for faster summation
    const size_t size = collector.areas.size();
    const f32* areas = collector.areas.data();
    
    // Use 8 accumulators for even better pipelining and to utilize more registers
    __m256 sum0 = _mm256_setzero_ps();
    // sum1 - sum6 omitted for brevity
    __m256 sum7 = _mm256_setzero_ps();
    
    // Process 64 elements at a time - further unrolled loop with prefetching
    size_t i = 0;
    
    // For very large arrays, first prefetch ahead
    if (size >= 128) {
        _mm_prefetch((const char*)&areas[64], _MM_HINT_T0);
        _mm_prefetch((const char*)&areas[96], _MM_HINT_T0);
    }
    
    for (; i + 63 < size; i += 64) {
        // Prefetch next iterations to L1 cache
        _mm_prefetch((const char*)&areas[i + 128], _MM_HINT_T0);
        _mm_prefetch((const char*)&areas[i + 160], _MM_HINT_T0);
        
        // Fully unrolled loop for 64 elements with 8 accumulators
        // This eliminates loop overhead and maximizes instruction-level parallelism
        sum0 = _mm256_add_ps(sum0, _mm256_loadu_ps(&areas[i]));
        // sum1 - sum6 omitted for brevity
        sum7 = _mm256_add_ps(sum7, _mm256_loadu_ps(&areas[i + 56]));
    }
    
    // Combine the 8 accumulators into 4
    sum0 = _mm256_add_ps(sum0, sum4);
    // sum1 - sum6 omitted for brevity
    sum3 = _mm256_add_ps(sum3, sum7);
    
    // Combine the 4 accumulators into 2
    sum0 = _mm256_add_ps(sum0, sum1);
    sum2 = _mm256_add_ps(sum2, sum3);
    
    // Combine the 2 accumulators into 1
    sum0 = _mm256_add_ps(sum0, sum2);
    
    // Now process 8 elements at a time for remaining data
    for (; i + 7 < size; i += 8) {
        sum0 = _mm256_add_ps(sum0, _mm256_loadu_ps(&areas[i]));
    }
    
    // Extract result from AVX register using more efficient horizontal sum
    __m128 high128 = _mm256_extractf128_ps(sum0, 1);
    __m128 low128 = _mm256_castps256_ps128(sum0);
    __m128 sum128 = _mm_add_ps(high128, low128);
    
    // Horizontal sum of 128-bit SSE vector - optimized version
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    f32 Accum = _mm_cvtss_f32(sum128);
    
    // Handle remaining elements
    for (; i < size; ++i) {
        Accum += areas[i];
    }
    
    return Accum;
}

```

I'll spare you the CornerAreaCollector function, which is similar boring. (You can find it in the code repository[^1].)

This implementation uses:

* **SIMD (AVX) instructions** for parallel processing of shape areas and weights
* **Loop unrolling** and **multiple accumulators** to maximize instruction-level parallelism
* **Cache prefetching** for large arrays to minimize memory latency
* **Fused Multiply-Add (FMA)** for efficient weighted sums


### Speed Comparison: Clean Code vs. Switch vs. Table

| Benchmark                | TotalArea (ms) | CornerArea (ms) | Speed-up vs OOP baseline |
|-------------------------|----------------|-----------------|--------------------------|
| Clean Code (OOP)        | 2.516         | 5.603           | 1.0x                     |
| Clean Code (4x unrolled)| 1.023         | 2.000           | 2.5x / 2.8x              |
| Switch-based            | 0.644         | 0.643           | 3.9x / 8.7x              |
| Switch-based (4x unrolled)| 0.379       | 0.531           | 6.6x / 10.6x             |
| Table-driven            | 0.639         | 0.655           | 3.9x / 8.6x              |
| Table-driven (4x unrolled)| 0.337       | 0.335           | 7.5x / 16.7x             |
| **Optimized Clean Code**| **0.035**     | **0.075**       | **72x / 75x**            |

These values were measured on a AMD Ryzen 7 8745H with GCC 13.3.0 and all optimizations enabled.


### Conclusion: Clean Code Can Be Fast

The dichotomy between clean code and performance is a false one, as demonstrated by our optimized clean code implementation that achieves 70x better performance while maintaining good software engineering principles. By separating concerns and applying targeted optimizations where they matter most, developers can create systems that are both clean and fast without sacrificing maintainability or extensibility.


## Epilogue

Another striking claim of Casey Muratori is:

> *Personally, I wouldn’t say that a switch statement is inherently less polymorphic than a vtable. They are just two different implementations of the same thing. But, the “clean” code rules say to prefer polymorphism to switch statements, so I’m using their terminology here, where clearly they don’t think a switch statement is polymorphic.*

While the discussion about maintainability and extensibility in part 3 shows that there is indeed a major difference between switch statements and virtual methods, I was puzzled by the speed of switch statements.

In the old days we tried to avoid big switch case statements also because of their speed penalty: "While a virtual method just has two jumps the switch has a lot of comparisons". Even with branch prediction there shall be a cross point, when the number of cases is so high that a vtable gets faster. I wrote a small benchmark program to find out where this cross point is. I stopped after 1000 case statements and still being significantly faster than a virtual method call.
Branch prediction seems to be incredibly efficient and raises the question if c++ compilers shall use a switch implementation for virtual methods.

[^1]: All benchmark code, implementations, and build instructions are available at: https://github.com/kochelmonster/articles/tree/master/cleancode
