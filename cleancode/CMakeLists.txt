cmake_minimum_required(VERSION 3.10)
project(cleancode_bench)

set(CMAKE_CXX_STANDARD 17)

# Add all optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx -mfma -mtune=native -flto -DNDEBUG -D__FMA__")
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

add_executable(bench
    src/bench.cpp
    src/clean_code.cpp
    src/switch_code.cpp
    src/table_code.cpp
    src/optimized_clean_code.cpp
)
