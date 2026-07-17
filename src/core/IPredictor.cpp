// ─────────────────────────────────────────────────────────────────────────────
// IPredictor.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/IPredictor.hpp>
#include <cmath>

namespace hypercore {

bool ByteDistribution::is_valid() const noexcept {
    f32 sum = 0.0f;
    for (f32 p : prob) {
        if (p < 0.0f) return false;
        sum += p;
    }
    // Allow small floating point drift
    return std::abs(sum - 1.0f) < 0.01f;
}

void ByteDistribution::normalize() noexcept {
    f32 sum = 0.0f;
    for (f32 p : prob) {
        sum += p;
    }
    
    if (sum > 0.0f) {
        f32 inv_sum = 1.0f / sum;
        for (f32& p : prob) {
            p *= inv_sum;
        }
    } else {
        // Fallback to uniform
        f32 uniform = 1.0f / 256.0f;
        for (f32& p : prob) {
            p = uniform;
        }
    }
}

QuantizedDistribution QuantizedDistribution::from(const ByteDistribution& dist) noexcept {
    QuantizedDistribution q;
    u32 total = 0;
    
    // First pass: simply multiply by TABLE_SIZE and floor
    for (u32 i = 0; i < 256; ++i) {
        u8 b = static_cast<u8>(i & 0xFF);
        u32 count = static_cast<u32>(dist[b] * static_cast<f32>(TABLE_SIZE));
        q[b] = count;
        total += count;
    }
    
    // Second pass: distribute the remainder (due to flooring)
    // To minimize error, we could distribute to the largest fractional parts,
    // but for simplicity we just add it to the most probable symbol.
    if (total < TABLE_SIZE) {
        u32 remainder = TABLE_SIZE - total;
        // Find max element
        u8 max_idx = 0;
        u32 max_val = q[static_cast<u8>(0)];
        for (u32 i = 1; i < 256; ++i) {
            u8 b = static_cast<u8>(i & 0xFF);
            if (q[b] > max_val) {
                max_val = q[b];
                max_idx = b;
            }
        }
        q[max_idx] += remainder;
    } else if (total > TABLE_SIZE) {
        // This shouldn't normally happen with flooring, but just in case
        u32 excess = total - TABLE_SIZE;
        // Find max element and subtract
        u8 max_idx = 0;
        u32 max_val = q[static_cast<u8>(0)];
        for (u32 i = 1; i < 256; ++i) {
            u8 b = static_cast<u8>(i & 0xFF);
            if (q[b] > max_val) {
                max_val = q[b];
                max_idx = b;
            }
        }
        if (q[max_idx] >= excess) {
            q[max_idx] -= excess;
        }
    }
    
    return q;
}

bool QuantizedDistribution::is_valid() const noexcept {
    u32 sum = 0;
    for (u32 c : freq) {
        sum += c;
    }
    return sum == TABLE_SIZE;
}

} // namespace hypercore
