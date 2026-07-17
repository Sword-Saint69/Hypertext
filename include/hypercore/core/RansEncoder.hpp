#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// RansEncoder.hpp — Range Asymmetric Numeral Systems encoder
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/IPredictor.hpp>
#include <hypercore/core/BitWriter.hpp>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
// RansEncoder compresses symbols given their dynamic probability distributions.
// Uses a 32-bit state, flushing bytes to BitWriter to prevent overflow.
// M (TABLE_SIZE) = 4096 (12 bits)
// State range [L, 2L-1] where L = 2^16.
// ─────────────────────────────────────────────────────────────────────────────
class RansEncoder {
public:
    explicit RansEncoder(BitWriter& writer);

    /// Encode a symbol based on the dynamic probability distribution
    void encode_symbol(u8 symbol, const QuantizedDistribution& dist);

    /// Flush the final rANS state into the bitstream
    void flush();

private:
    // rANS state
    u32 m_state;
    BitWriter& m_writer;

    // L must be > M to ensure proper state bounding. 
    // M is 4096 (2^12). We'll use L = 1 << 16 (65536).
    static constexpr u32 RANS_L = 1u << 16;
};

} // namespace hypercore
