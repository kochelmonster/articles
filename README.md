# Clean Code vs. Optimization: Debunking the False Dichotomy

This repository contains the full article and benchmark code for "Clean Code vs. Optimization: Debunking the False Dichotomy" - a comprehensive response to Casey Muratori's "Clean Code, Horrible Performance".

## 📖 Read the Full Article

**[→ Read the complete article here](cleancode/article.md)**

## Overview

This article challenges the false dichotomy between clean code and performance. Through rigorous benchmarking and analysis, it demonstrates that:

- Clean code and high performance are **not mutually exclusive**
- With proper optimization techniques, clean object-oriented code achieves **72x performance improvement** over baseline
- Modern branch prediction and SIMD optimizations enable the best of both worlds
- Strategic separation of concerns allows targeted optimization without sacrificing maintainability

## Key Results

| Approach | TotalArea (ms) | CornerArea (ms) | Speedup |
|----------|----------------|-----------------|---------|
| Clean Code (OOP) | 2.516 | 5.603 | 1.0x (baseline) |
| Switch-based | 0.644 | 0.643 | 3.9x / 8.7x |
| Table-driven | 0.639 | 0.655 | 3.9x / 8.6x |
| **Optimized Clean Code** | **0.035** | **0.075** | **72x / 75x** |

*Measured on AMD Ryzen 7 8745H with GCC 13.3.0*

## Repository Contents

### [cleancode/](cleancode/)

Complete benchmark implementation and analysis:

- **[article.md](cleancode/article.md)** - Full article with detailed analysis
- **[README.md](cleancode/README.md)** - Benchmark documentation and build instructions
- **src/** - All implementation variants (OOP, switch, table, optimized)
- **include/** - Shape class definitions
- **CMakeLists.txt** - Build configuration

## Article Structure

1. **Code Analysis** - Examining the three implementations from the original critique
2. **Context Setting** - Professional software development goals and quality attributes
3. **Hidden Costs** - What optimizations sacrifice in maintainability and extensibility
4. **Performance Evaluation** - Critical examination of performance claims
5. **Synthesis** - Optimized clean code that exceeds all other approaches by 4x

## Building and Running

```bash
cd cleancode
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bench
```

See [cleancode/README.md](cleancode/README.md) for detailed build instructions.

## Contributing

Feel free to submit issues or pull requests to improve the benchmarks or add new implementations.

## License

This code is provided for educational and research purposes. See the article for detailed explanations of the techniques used.
