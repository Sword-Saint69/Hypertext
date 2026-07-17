// ─────────────────────────────────────────────────────────────────────────────
// RansEncoder.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/RansEncoder.hpp>
#include <cassert>

#ifdef HYPERCORE_HAS_AVX2
#include <immintrin.h>
#endif

namespace hypercore {

RansEncoder::RansEncoder(BitWriter& writer)
    : m_state(RANS_L), m_writer(writer) {
}

void RansEncoder::encode_symbol(u8 symbol, const QuantizedDistribution& dist) {
    u32 freq = dist[symbol];
    if (freq == 0) {
        // Fallback for zero probability if smoothing failed
        freq = 1;
    }
    
    // Calculate cumulative frequency (could be optimized with a Fenwick tree or SIMD, 
    // but a simple loop is fine for the CPU prototype)
    u32 cum_freq = 0;
#ifdef HYPERCORE_HAS_AVX2
    // Vectorized cumulative sum using AVX2.
    // The QuantizedDistribution::freq array has exactly 256 elements.
    // We sum the frequencies from 0 to symbol-1.
    if (symbol > 0) {
        __m256i v_sum = _mm256_setzero_si256();
        const u32* freq_ptr = dist.freq.data();
        
        // Sum full 8-element chunks
        u32 i = 0;
        for (; i + 8 <= symbol; i += 8) {
            __m256i v_freq = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(freq_ptr + i));
            v_sum = _mm256_add_epi32(v_sum, v_freq);
        }
        
        // Horizontal sum of the accumulated vector
        // Extract 128-bit lanes
        __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(v_sum), _mm256_extracti128_si256(v_sum, 1));
        // Add adjacent pairs
        sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1, 0, 3, 2)));
        sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(2, 3, 0, 1)));
        cum_freq = _mm_cvtsi128_si32(sum128);
        
        // Process remainder elements
        for (; i < symbol; ++i) {
            cum_freq += freq_ptr[i];
        }
    }
#else
    for (u32 i = 0; i < symbol; ++i) {
        cum_freq += dist.freq[i];
    }
#endif
    
    // Normalize state: if x >= M * (L / freq), we must write bits to prevent overflow
    // In standard rANS, x_max = 2^32 - 1. We'll flush bytes (8 bits).
    // Our upper bound is RANS_L * (QuantizedDistribution::TABLE_SIZE / freq).
    // Actually, to keep it simple, we normalize to stay in [L, 2L-1] scaled by freq.
    // Exact condition: max_state = ((RANS_L * 2) / QuantizedDistribution::TABLE_SIZE) * freq
    
    // But since M = 4096, L = 65536, L/M = 16.
    // So max_state = 32 * freq.
    
    // More robust normalization (standard rANS):
    // Max x before encode is (x_max / M) * freq. 
    // If we use 32-bit state, x_max = ~4 billion. 
    // M = 4096. x_max / M = ~1 million.
    // So if x >= (1 << 20) * freq, we output a byte.
    // Let's use a standard bound:
    u32 bound = ( (1u << 31) / QuantizedDistribution::TABLE_SIZE ) * freq;
    
    while (m_state >= bound) {
        m_writer.write_bits(m_state & 0xFF, 8);
        m_state >>= 8;
    }
    
    // Encode step: x_next = (x / freq) * M + cum_freq + (x % freq)
    m_state = (m_state / freq) * QuantizedDistribution::TABLE_SIZE + cum_freq + (m_state % freq);
}

void RansEncoder::flush() {
    // Write the final 32-bit state to the bitstream
    m_writer.write_bits(m_state, 32);
    m_writer.flush();
}

} // namespace hypercore
