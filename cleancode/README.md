# Clean Code vs. Performance Benchmark

This repository contains the benchmark code and implementations used in the article "Clean Code vs. Optimization: Debunking the False Dichotomy" - a response to Casey Muratori's "Clean Code, Horrible Performance".

## Overview

The project demonstrates that clean code and high performance are not mutually exclusive by implementing and benchmarking four different approaches to shape area calculations:

1. **Clean Object-Oriented Code** - Using virtual functions and polymorphism
2. **Switch-based Code** - Procedural approach with switch statements  
3. **Table-driven Code** - Using precomputed coefficient tables
4. **Optimized Clean Code** - Clean architecture with SIMD optimizations

## Key Findings

Our benchmarks show that with proper optimization techniques, clean code can achieve **72x performance improvement** over the baseline while maintaining all the benefits of object-oriented design:

| Approach | TotalArea (ms) | CornerArea (ms) | Speedup |
|----------|----------------|-----------------|---------|
| Clean Code (OOP) | 2.516 | 5.603 | 1.0x (baseline) |
| Switch-based | 0.644 | 0.643 | 3.9x / 8.7x |
| Table-driven | 0.639 | 0.655 | 3.9x / 8.6x |
| **Optimized Clean Code** | **0.035** | **0.075** | **72x / 75x** |

## Building and Running

### Prerequisites

- GCC 13.3+ with AVX support
- CMake 3.10+
- Linux environment (tested on AMD Ryzen 7 8745H)

### Build Instructions

```bash
# Configure the build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build all targets
cmake --build build

# Run the main benchmark
./build/bench
```

### Available Executables

- `bench` - Main benchmark comparing all approaches
- `bench_switch_vs_virtual` - Switch vs virtual function comparison (up to 100 cases)
- `extreme_switch_vs_virtual` - Extended comparison (up to 200 cases)  
- `ultra_switch_vs_virtual` - Ultimate test (up to 1000 cases)

## Project Structure

```
cleancode/
├── src/
│   ├── bench.cpp                      # Main benchmark
│   ├── clean_code.cpp                 # OOP implementation
│   ├── switch_code.cpp                # Switch-based implementation
│   ├── table_code.cpp                 # Table-driven implementation
│   ├── optimized_clean_code.cpp       # SIMD-optimized clean code
│   ├── bench_switch_vs_virtual.cpp    # Switch vs virtual comparison
│   ├── extreme_switch_vs_virtual.cpp  # Extended switch comparison
│   └── ultra_switch_vs_virtual.cpp    # Ultimate switch test
├── include/
│   └── shapes.h                       # Shape class definitions
├── CMakeLists.txt                     # Build configuration
├── article.md                         # Full article text
└── README.md                          # This file
```

## Key Insights Demonstrated

### 1. Maintainability vs Performance is a False Dichotomy
The optimized clean code approach proves you can have both clean architecture and exceptional performance.

### 2. Branch Prediction is Incredibly Effective
Even with 1000+ switch cases, switch statements remain faster than virtual functions due to modern CPU branch prediction.

### 3. SIMD + Clean Architecture = Best of Both Worlds
By separating concerns (collection vs computation), we achieve:
- Clean, extensible object model
- Blazing fast SIMD computations
- Easy portability to different architectures

### 4. Optimization Should Be Targeted
Instead of abandoning clean code principles, apply optimizations where they matter most while preserving good software engineering practices.

## Benchmark Details

All benchmarks process 1 million shapes with equal distribution of squares, rectangles, triangles, and circles. Tests are run with:
- GCC 13.3.0 with -O3 optimization
- AVX2 SIMD instructions enabled
- Multiple iterations for statistical accuracy

## Contributing

Feel free to submit issues or pull requests to improve the benchmarks or add new implementations.

## Related Article

For the complete analysis and discussion of these results, see [article.md](article.md).

## License

This code is provided for educational and research purposes. See the article for detailed explanations of the techniques used.



