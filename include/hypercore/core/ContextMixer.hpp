#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ContextMixer.hpp — Combines multiple probability distributions
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/IPredictor.hpp>
#include <vector>
#include <memory>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
// Stage 4: Context Mixer (CPU Prototype)
// ─────────────────────────────────────────────────────────────────────────────
class ContextMixer {
public:
    ContextMixer() = default;

    /// Add a predictor to the mixer (mixer takes ownership)
    void add_predictor(std::unique_ptr<IPredictor> predictor);

    /// Predict the probability distribution of the next byte
    [[nodiscard]] ByteDistribution predict(const PredictorContext& ctx);

    /// Update all models after observing the actual byte
    void update(const PredictorContext& ctx, u8 actual);

    /// Reset all models to initial state
    void reset();

private:
    std::vector<std::unique_ptr<IPredictor>> m_predictors;
};

} // namespace hypercore
