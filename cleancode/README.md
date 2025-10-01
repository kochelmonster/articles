# Clean Code Horrible Performance Benchmark

This project benchmarks three versions of shape area calculation as described in Casey Muratori's article "Clean Code, Horrible Performance":

1. **Clean Code (OOP/Polymorphism)**
2. **Switch Statement Version**
3. **Table-Driven Version**

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
./bench
```

## Files
- `src/clean_code.cpp`: OOP/Polymorphism version
- `src/switch_code.cpp`: Switch statement version
- `src/table_code.cpp`: Table-driven version
- `src/bench.cpp`: Benchmark runner
- `include/shapes.h`: Common shape definitions

- Mängel in der Architektur
- Neue Aggregatoren?
- Neue Formen?
- Bibliothek keine Erweitebarkeit für Kunden


Resterampe:
- Testbarkeit ist nicht wesentlich beeinflußt
- CahtGPT behautptet drehte sich im Kreis bei Testbarkeit.

switch case ist polymprophismus