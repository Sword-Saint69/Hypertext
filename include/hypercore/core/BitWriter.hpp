#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BitWriter.hpp — Simple bit-level stream writer
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <vector>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
// BitWriter packs bits into a dynamically growing byte buffer.
// It writes bits from least-significant to most-significant bit within a byte.
// ─────────────────────────────────────────────────────────────────────────────
class BitWriter {
public:
    BitWriter() = default;

    /// Write `num_bits` from the least significant bits of `value`
    void write_bits(u64 value, u32 num_bits);

    /// Flush any remaining bits into the buffer, padding with 0
    void flush();

    /// Get the underlying byte buffer
    [[nodiscard]] const std::vector<u8>& get_buffer() const noexcept { return m_buffer; }

    /// Total number of bits written
    [[nodiscard]] u64 get_total_bits() const noexcept { return m_total_bits; }

private:
    std::vector<u8> m_buffer;
    u64 m_bit_accumulator = 0;
    u32 m_bits_in_accumulator = 0;
    u64 m_total_bits = 0;
};

} // namespace hypercore
