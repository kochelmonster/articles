# Clean Code vs. Optimization: Debunking the False Dichotomy

## Prologue: Reexamining "Clean Code, Horrible Performance"

This article responds to Casey Muratori's provocative piece "Clean Code, Horrible Performance" ([https://www.computerenhance.com/p/clean-code-horrible-performance](https://www.computerenhance.com/p/clean-code-horrible-performance)). Muratori argues that Robert C. Martin's "clean code" principles fundamentally compromise software performance, using shape area calculations to demonstrate his point. His central thesis is stark: clean code methodology makes software dramatically slower, with four of the five core principles proving detrimental to performance—sometimes by a factor of 15 or more.

Muratori contends that sacrificing a decade of hardware performance improvements for programmer convenience is unacceptable. While acknowledging that code organization and maintainability are worthy goals, he argues that current "clean code" practices fail to achieve them without imposing severe performance penalties.

This response unfolds across five parts:

1. **Code Analysis**: Examining the three implementations Muratori presented as his case study
2. **Context Setting**: Discussing professional software development goals and where performance fits among quality attributes  
3. **Hidden Costs**: Analyzing what Muratori's optimizations sacrifice in terms of maintainability and extensibility
4. **Performance Evaluation**: Critically examining the performance claims and their broader implications
5. **Synthesis**: Presenting a clean code solution that achieves performance exceeding even the "optimized" implementations by over 4x, proving that clean code and performance need not be mutually exclusive

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

Object-oriented programming and clean coding practices weren't developed as academic exercises—they emerged as practical solutions to real problems that plagued software projects throughout their lifecycles, particularly around maintainability and extensibility.

In the 1990s, while working for several banks, I witnessed firsthand how seemingly minor oversights could spiral into catastrophic costs. The programmers who wrote the boot routines for the mainframes had long since retired or passed away, leaving behind impenetrable legacy code. No one dared shut down the systems for fear they wouldn't boot again. Banks spent millions on increasingly obsolete hardware just to keep these digital time bombs running.

This illustrates a crucial point: clean coding isn't about programmer "convenience"—it's about economic efficiency. Every micro-optimization demands additional development time, specialized testing, and extensive documentation. Optimizations often rely on clever tricks that sacrifice readability and maintainability. The critical question becomes: is your customer willing to pay these hidden costs for speed improvements they may never notice?

Runtime performance is undeniably important, but it must be weighed against other quality attributes. Professional software development is fundamentally about maximizing value while minimizing cost—delivering rich functionality with reasonable effort. "Functionality" here encompasses not just features, but the full spectrum of software qualities: reliability, maintainability, extensibility, portability, testability, and yes, execution speed.

Software needs to be "fast enough" for its context. For most user-facing applications, this means avoiding perceptible delays. In high-frequency trading or real-time systems, microseconds matter. But in many domains, obsessing over performance that users will never notice represents a costly misallocation of resources.

The principle is simple: optimize when performance becomes a genuine constraint, not preemptively. To paraphrase Einstein: "Make software as fast as needed, but not faster."

## Part 3: Maintainability and Extensibility - The Hidden Costs of Optimization

Let's examine how Muratori's optimizations fare during typical software evolution.

### The Burden of Algorithm Changes

Consider a seemingly simple change: upgrading triangle area calculation from the basic base×height formula to Heron's formula, which requires all three side lengths.

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

Because Muratori restructured the entire architecture for runtime performance, a simple algorithmic change to one shape's method cascades into a complete program rewrite. The table-driven approach fares even worse—it fundamentally breaks under this change.

### The Library Extensibility Challenge

The maintainability problems are clear, but what about extensibility? The clean code approach represents a common library scenario where third parties should be able to add new shape types. Let's test this by adding a hexagon shape.

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

**In switch code**, every function must be modified to handle hexagons. Library vendors would need to provide full source code for customers to add shapes—but what happens when you release the next version? Customer modifications become merge conflicts.

**In table code**, extension requires either modifying the library source or creating wrapper layers that defeat the original optimization.

Muratori's optimizations extract a steep price: they sacrifice both maintainability and extensibility. This forces a strategic choice: invest in flexible, maintainable software that may require faster hardware (which becomes cheaper annually), or lock yourself into highly optimized but inflexible code frozen in time?


## Part 4: Performance Claims Analysis

### The Fundamental Methodological Flaw

Before examining the performance numbers, we must address a critical issue with Muratori's comparison methodology: **he compares demo-quality clean code against highly optimized implementations**. This is fundamentally unfair—like comparing a bicycle against a Formula 1 race car and concluding that bicycles are inherently slow.

Muratori's "clean code" baseline appears designed for readability and teaching purposes, not performance. It lacks even basic optimizations that would be standard in any performance-sensitive clean code implementation:
- No loop unrolling
- No compiler optimization flags consideration  
- No data structure optimization
- No algorithmic improvements
- No consideration of cache-friendly patterns

Meanwhile, his "optimized" versions employ aggressive techniques:
- Manual loop unrolling
- Processor-specific optimizations
- Cache-optimized data layouts
- Branch prediction exploitation

**A fair comparison would pit optimized clean code against optimized procedural code**, not demo code against production-tuned implementations. This methodological flaw undermines the entire premise that clean code is inherently slower.

### Empirical Verification

Despite this methodological issue, let's examine the actual performance claims. Muratori's performance claims lack reproducible benchmark code, reporting that his optimized versions run 25 times faster than the clean code baseline. To verify these claims, I implemented his code samples and created comprehensive benchmarks. Testing on an AMD Ryzen 7 8745H yielded these results:

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

The results show the best optimized version achieving 16x improvement over baseline clean code—or 6.5x compared to unrolled clean code. While significant, this falls short of Muratori's claimed 25x speedup.

More importantly, these optimizations exploit processor-specific features:

- Loop unrolling enables pipelining
- Switch-case code leverages branch prediction  
- Table-driven code exploits cache optimization

These advantages only materialize on processors supporting specific optimization techniques. While most modern processors include these features, future architectures might render them obsolete—or even favor different approaches like optimized indirection. Today's micro-optimizations could become tomorrow's performance bottlenecks.

The platform dependency runs deeper than architecture evolution. On resource-constrained processors, these optimizations can backfire: I once achieved 150% speedup on a PIC microcontroller by replacing switch statements with function pointer arrays—the exact opposite of Muratori's recommendation.

Muratori makes a particularly striking analogy:

> *To put that in hardware terms, it would be like taking an iPhone 14 Pro Max and reducing it to an iPhone 11 Pro Max. It's three or four years of hardware evolution erased because somebody said to use polymorphism instead of switch statements.*

This argument contains internal contradictions. The optimizations he advocates only work effectively on modern hardware—the very hardware he claims is being "erased." Furthermore, if we accepted this logic consistently, we'd need to ban all interpreted languages (which would regress an iPhone 14 to iPhone 3 performance) and abandon high-level languages entirely in favor of hand-optimized assembly. 

### The Broader Performance Picture

Modern compilers increasingly eliminate performance gaps that once required manual intervention. Advanced optimization techniques—inlining, dead code elimination, profile-guided optimization—often match or exceed hand-tuned code while maintaining portability and forward compatibility.

Real-world performance bottlenecks rarely lie in computational hotpaths like shape calculations. Database queries, network latency, and I/O operations typically dominate performance profiles. Optimizing the wrong component—no matter how elegantly—delivers negligible user-visible improvements.

Algorithmic improvements consistently outweigh micro-optimizations. Converting an O(n²) algorithm to O(n log n) dwarfs any low-level tweaking benefits. Moreover, clean, portable code positions itself to benefit from future compiler advances and hardware evolution without requiring architectural rewrites.


## Part 5: Optimized Clean Code – The Best of Both Worlds

This section demonstrates how to preserve clean, object-oriented interfaces while achieving exceptional performance through strategic optimization. The approach separates three distinct concerns:

1. **Data Collection**: Maintaining clean object-oriented interfaces for domain logic
2. **Precomputation**: Extracting and caching invariant data once per shape  
3. **Vectorized Aggregation**: Performing bulk calculations using SIMD instructions

Crucially, only the final step involves hardware-specific optimizations—the domain model and data collection remain clean, maintainable, and extensible.

### 5.1 Collectors: Bridging Clean Code and Performance

The collector pattern creates a thin adaptation layer between the clean object interface and optimized computation. These classes extract precisely what the performance-critical loops require:

* `AreaCollector`: stores raw areas in a contiguous `std::vector<float>`
* `CornerCollector`: stores areas plus precomputed weight `1/(1+corner_count)`, paying the expensive division and virtual call costs once per shape rather than inside the SIMD loop

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

These collectors preserve domain model clarity while organizing data for optimal CPU utilization—the best of both architectural worlds.

### 5.2 Precomputation: Pay Once, Benefit Repeatedly  

The setup phase maintains the familiar polymorphic shape interface while simultaneously feeding the collectors. Virtual method dispatch occurs exactly once per shape during data collection—subsequent aggregation becomes pure vectorized arithmetic.

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

This architectural transformation splits virtual-dispatch-heavy reduction into two clean phases: gather and crunch. Porting to different architectures or vector widths leaves the gathering logic completely untouched.

### 5.3 Vectorized Aggregation: Where Performance Magic Happens

The aggregation layer operates on contiguous float arrays, deploying the full arsenal of modern CPU optimization: AVX 256-bit intrinsics for 8-wide parallelism, aggressive loop unrolling, multiple accumulators to hide pipeline latency, intelligent prefetching for cache optimization, and fused multiply-add (FMA) operations.

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

The CornerAreaCollector follows similar patterns with additional complexity for weighted calculations. (Complete implementation details are available in the code repository[^1].)

The optimization arsenal deployed includes:

* **SIMD (AVX) Instructions**: 8-way parallel processing of floating-point data  
* **Aggressive Loop Unrolling**: Multiple accumulators to maximize instruction-level parallelism
* **Intelligent Prefetching**: Proactive cache loading for large datasets
* **Fused Multiply-Add (FMA)**: Single-instruction `a*b + c` operations for weighted sums


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


## Conclusion: Transcending False Dichotomies

The supposed trade-off between clean code and performance represents a false choice. Our optimized implementation delivers 70x performance improvement while preserving sound software engineering principles. The key insight: strategic separation of concerns allows targeted optimization where it matters most, creating systems that are simultaneously clean and fast without sacrificing maintainability or extensibility.

**The path forward combines:**
- **Clean interfaces** that preserve domain clarity and enable extension
- **Strategic optimization** applied only where performance demands it  
- **Architectural separation** between business logic and computational efficiency
- **Modern tooling** (SIMD, advanced compilers) used judiciously rather than universally

This approach delivers genuine value: software that performs exceptionally while remaining adaptable to changing requirements and maintainable across its entire lifecycle.


---

## Epilogue: Lessons from the Benchmarking Journey

Writing this response uncovered surprising insights that extend beyond the main argument. One particular claim from Muratori sparked additional investigation:

> *Personally, I wouldn't say that a switch statement is inherently less polymorphic than a vtable. They are just two different implementations of the same thing. But, the "clean" code rules say to prefer polymorphism to switch statements, so I'm using their terminology here, where clearly they don't think a switch statement is polymorphic.*

While Part 3 demonstrates fundamental architectural differences between switch statements and virtual methods, Muratori's performance observations about switches proved intriguingly counterintuitive.

### The Branch Prediction Revelation

Traditional wisdom suggested avoiding large switch statements due to performance penalties—multiple comparisons versus virtual methods' simple double indirection. Even accounting for branch prediction, there should theoretically be a crossover point where virtual tables become faster than switches.

Determined to find this crossover, I wrote benchmark programs scaling from small switch statements up to 1000 cases. The results were startling: **switches remained significantly faster even at 1000 cases**. Modern branch prediction has become remarkably sophisticated, challenging decades-old performance assumptions and raising intriguing questions about optimal compiler implementation strategies for virtual method dispatch.

This discovery reinforces the importance of empirical measurement over inherited assumptions—the performance landscape continues evolving with hardware advances.

[^1]: All benchmark code, implementations, and build instructions are available at: https://github.com/kochelmonster/articles/tree/master/cleancode
