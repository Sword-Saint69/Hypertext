// ─────────────────────────────────────────────────────────────────────────────
// CharacterPredictor.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/CharacterPredictor.hpp>

namespace hypercore {

CharacterPredictor::CharacterPredictor() {
    reset();
}

void CharacterPredictor::reset() noexcept {
    // We start with a count of 1 for all transitions to provide a baseline
    // uniform probability (additive smoothing / Laplace smoothing) and prevent 
    // zero probabilities.
    m_counts.assign(256, std::vector<u32>(256, 1));
    m_totals.assign(256, 256); // 256 possibilities each starting with count 1
}

ByteDistribution CharacterPredictor::predict(const PredictorContext& ctx) {
    ByteDistribution dist;
    
    u8 prev_byte = 0;
    if (!ctx.history.empty()) {
        prev_byte = ctx.history.back();
    }

    const auto& counts = m_counts[prev_byte];
    u32 total = m_totals[prev_byte];
    f32 inv_total = 1.0f / static_cast<f32>(total);

    for (u32 i = 0; i < 256; ++i) {
        dist[static_cast<u8>(i)] = static_cast<f32>(counts[i]) * inv_total;
    }

    // dist.normalize() is implicitly handled because the math guarantees it sums to 1.
    return dist;
}

void CharacterPredictor::update(const PredictorContext& ctx, u8 actual) {
    u8 prev_byte = 0;
    if (!ctx.history.empty()) {
        prev_byte = ctx.history.back();
    }

    m_counts[prev_byte][actual]++;
    m_totals[prev_byte]++;
    
    // To prevent overflow in long files, we could periodically halve all counts.
    // We'll add a simple scaling logic here to bound the maximum frequency.
    if (m_totals[prev_byte] >= 16384) {
        u32 new_total = 0;
        for (int i = 0; i < 256; ++i) {
            // Halve the count, but ensure it never drops below 1
            u32 c = m_counts[prev_byte][i];
            c = (c >> 1) | 1; 
            m_counts[prev_byte][i] = c;
            new_total += c;
        }
        m_totals[prev_byte] = new_total;
    }
}

} // namespace hypercore
