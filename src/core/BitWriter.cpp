// ─────────────────────────────────────────────────────────────────────────────
// BitWriter.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/BitWriter.hpp>

namespace hypercore {

void BitWriter::write_bits(u64 value, u32 num_bits) {
    if (num_bits == 0) return;
    
    // Mask out only the relevant bits
    u64 mask = (1ULL << num_bits) - 1;
    value &= mask;
    
    // Pack into accumulator
    m_bit_accumulator |= (value << m_bits_in_accumulator);
    m_bits_in_accumulator += num_bits;
    m_total_bits += num_bits;
    
    // Flush complete bytes
    while (m_bits_in_accumulator >= 8) {
        m_buffer.push_back(static_cast<u8>(m_bit_accumulator & 0xFF));
        m_bit_accumulator >>= 8;
        m_bits_in_accumulator -= 8;
    }
}

void BitWriter::flush() {
    if (m_bits_in_accumulator > 0) {
        m_buffer.push_back(static_cast<u8>(m_bit_accumulator & 0xFF));
        m_bit_accumulator = 0;
        m_bits_in_accumulator = 0;
    }
}

} // namespace hypercore
