// ─────────────────────────────────────────────────────────────────────────────
// RansEncoder.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/RansEncoder.hpp>
#include <cassert>

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
    for (u32 i = 0; i < symbol; ++i) {
        cum_freq += dist[static_cast<u8>(i)];
    }
    
    // Normalize state: if x >= M * (L / freq), we must write bits to prevent overflow
    // In standard rANS, x_max = 2^32 - 1. We'll flush bytes (8 bits).
    // Our upper bound is RANS_L * (QuantizedDistribution::TABLE_SIZE / freq).
    // Actually, to keep it simple, we normalize to stay in [L, 2L-1] scaled by freq.
    // The exact condition:
    u32 max_state = ((RANS_L * 2) / QuantizedDistribution::TABLE_SIZE) * freq;
    
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
