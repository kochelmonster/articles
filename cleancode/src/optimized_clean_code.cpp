#include "shapes.h"
#include <algorithm>
#include <immintrin.h> // For SIMD vector instructions


// Collector-based functions for benchmarking
f32 TotalAreaCollector(AreaCollector& collector) {
    // Use SIMD for faster summation
    const size_t size = collector.areas.size();
    const f32* areas = collector.areas.data();
    
    // Use 8 accumulators for even better pipelining and to utilize more registers
    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();
    __m256 sum4 = _mm256_setzero_ps();
    __m256 sum5 = _mm256_setzero_ps();
    __m256 sum6 = _mm256_setzero_ps();
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
        sum1 = _mm256_add_ps(sum1, _mm256_loadu_ps(&areas[i + 8]));
        sum2 = _mm256_add_ps(sum2, _mm256_loadu_ps(&areas[i + 16]));
        sum3 = _mm256_add_ps(sum3, _mm256_loadu_ps(&areas[i + 24]));
        sum4 = _mm256_add_ps(sum4, _mm256_loadu_ps(&areas[i + 32]));
        sum5 = _mm256_add_ps(sum5, _mm256_loadu_ps(&areas[i + 40]));
        sum6 = _mm256_add_ps(sum6, _mm256_loadu_ps(&areas[i + 48]));
        sum7 = _mm256_add_ps(sum7, _mm256_loadu_ps(&areas[i + 56]));
    }
    
    // Combine the 8 accumulators into 4
    sum0 = _mm256_add_ps(sum0, sum4);
    sum1 = _mm256_add_ps(sum1, sum5);
    sum2 = _mm256_add_ps(sum2, sum6);
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

// Optimized corner area collector using precomputed weights and FMA if available
f32 CornerAreaCollector(CornerCollector& collector) {
    const size_t size = collector.areas.size();
    const f32* areas = collector.areas.data();
    const f32* weights = collector.weights.data();
    
    // Use 8 accumulators for better pipelining
    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();
    __m256 sum4 = _mm256_setzero_ps();
    __m256 sum5 = _mm256_setzero_ps();
    __m256 sum6 = _mm256_setzero_ps();
    __m256 sum7 = _mm256_setzero_ps();
    
    // Process 64 elements at a time with prefetching
    size_t i = 0;
    
    // For very large arrays, first prefetch ahead
    if (size >= 128) {
        _mm_prefetch((const char*)&areas[64], _MM_HINT_T0);
        _mm_prefetch((const char*)&areas[64+64], _MM_HINT_T0);
        _mm_prefetch((const char*)&weights[64], _MM_HINT_T0);
        _mm_prefetch((const char*)&weights[64+64], _MM_HINT_T0);
    }
    
    for (; i + 63 < size; i += 64) {
        // Prefetch next iterations to L1 cache
        _mm_prefetch((const char*)&areas[i + 128], _MM_HINT_T0);
        _mm_prefetch((const char*)&areas[i + 160], _MM_HINT_T0);
        _mm_prefetch((const char*)&weights[i + 128], _MM_HINT_T0);
        _mm_prefetch((const char*)&weights[i + 160], _MM_HINT_T0);
        
        // First 8 elements
        __m256 area_vec0 = _mm256_loadu_ps(&areas[i]);
        __m256 weight_vec0 = _mm256_loadu_ps(&weights[i]);
        
        // Next 8 elements
        __m256 area_vec1 = _mm256_loadu_ps(&areas[i + 8]);
        __m256 weight_vec1 = _mm256_loadu_ps(&weights[i + 8]);
        
        // Next 8 elements
        __m256 area_vec2 = _mm256_loadu_ps(&areas[i + 16]);
        __m256 weight_vec2 = _mm256_loadu_ps(&weights[i + 16]);
        
        // Next 8 elements
        __m256 area_vec3 = _mm256_loadu_ps(&areas[i + 24]);
        __m256 weight_vec3 = _mm256_loadu_ps(&weights[i + 24]);
        
        // Next 8 elements
        __m256 area_vec4 = _mm256_loadu_ps(&areas[i + 32]);
        __m256 weight_vec4 = _mm256_loadu_ps(&weights[i + 32]);
        
        // Next 8 elements
        __m256 area_vec5 = _mm256_loadu_ps(&areas[i + 40]);
        __m256 weight_vec5 = _mm256_loadu_ps(&weights[i + 40]);
        
        // Next 8 elements
        __m256 area_vec6 = _mm256_loadu_ps(&areas[i + 48]);
        __m256 weight_vec6 = _mm256_loadu_ps(&weights[i + 48]);
        
        // Next 8 elements
        __m256 area_vec7 = _mm256_loadu_ps(&areas[i + 56]);
        __m256 weight_vec7 = _mm256_loadu_ps(&weights[i + 56]);
        
        // Multiply and accumulate using 8 independent accumulators with FMA
        // FMA computes a*b+c in a single instruction with a single rounding step
        sum0 = _mm256_fmadd_ps(area_vec0, weight_vec0, sum0);
        sum1 = _mm256_fmadd_ps(area_vec1, weight_vec1, sum1);
        sum2 = _mm256_fmadd_ps(area_vec2, weight_vec2, sum2);
        sum3 = _mm256_fmadd_ps(area_vec3, weight_vec3, sum3);
        sum4 = _mm256_fmadd_ps(area_vec4, weight_vec4, sum4);
        sum5 = _mm256_fmadd_ps(area_vec5, weight_vec5, sum5);
        sum6 = _mm256_fmadd_ps(area_vec6, weight_vec6, sum6);
        sum7 = _mm256_fmadd_ps(area_vec7, weight_vec7, sum7);
    }
    
    // Combine the 8 accumulators into 4
    sum0 = _mm256_add_ps(sum0, sum4);
    sum1 = _mm256_add_ps(sum1, sum5);
    sum2 = _mm256_add_ps(sum2, sum6);
    sum3 = _mm256_add_ps(sum3, sum7);
    
    // Combine the 4 accumulators into 2
    sum0 = _mm256_add_ps(sum0, sum1);
    sum2 = _mm256_add_ps(sum2, sum3);
    
    // Combine the 2 accumulators into 1
    sum0 = _mm256_add_ps(sum0, sum2);
    
    // Process remaining 8-element chunks
    for (; i + 7 < size; i += 8) {
        __m256 area_vec = _mm256_loadu_ps(&areas[i]);
        __m256 weight_vec = _mm256_loadu_ps(&weights[i]);
        sum0 = _mm256_fmadd_ps(area_vec, weight_vec, sum0);
    }
    
    // Horizontal sum using efficient hadd instructions
    __m128 high128 = _mm256_extractf128_ps(sum0, 1);
    __m128 low128 = _mm256_castps256_ps128(sum0);
    __m128 sum128 = _mm_add_ps(high128, low128);
    
    // More efficient horizontal sum with hadd
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    f32 Accum = _mm_cvtss_f32(sum128);
    
    // Handle remaining elements
    for (; i < size; ++i) {
        Accum += areas[i] * weights[i];
    }
    
    return Accum;
}
