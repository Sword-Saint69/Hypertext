#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// CharacterPredictor.hpp — Order-1 byte transition model
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/IPredictor.hpp>
#include <vector>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
// Simple Order-1 Predictor: maintains frequency counts of the next byte given
// the immediately preceding byte.
// ─────────────────────────────────────────────────────────────────────────────
class CharacterPredictor final : public IPredictor {
public:
    CharacterPredictor();

    [[nodiscard]] ByteDistribution predict(const PredictorContext& ctx) override;
    void update(const PredictorContext& ctx, u8 actual) override;
    void reset() noexcept override;

    [[nodiscard]] PredictorType type() const noexcept override { return PredictorType::Character; }
    [[nodiscard]] std::string_view name() const noexcept override { return "CharacterPredictor (Order-1)"; }
    [[nodiscard]] u32 order() const noexcept override { return 1; }
    [[nodiscard]] u64 memory_bytes() const noexcept override { return 256 * 256 * sizeof(u32); }

private:
    // counts[prev_byte][next_byte]
    std::vector<std::vector<u32>> m_counts;
    
    // sum of all counts for a given prev_byte (used to calculate probabilities)
    std::vector<u32> m_totals;
};

} // namespace hypercore
