// ─────────────────────────────────────────────────────────────────────────────
// ContextMixer.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/ContextMixer.hpp>
#include <cassert>

namespace hypercore {

void ContextMixer::add_predictor(std::unique_ptr<IPredictor> predictor) {
    if (predictor) {
        m_predictors.push_back(std::move(predictor));
    }
}

ByteDistribution ContextMixer::predict(const PredictorContext& ctx) {
    ByteDistribution mixed;
    
    if (m_predictors.empty()) {
        // Uniform distribution fallback
        f32 uniform = 1.0f / 256.0f;
        for (u32 i = 0; i < 256; ++i) {
            mixed[static_cast<u8>(i)] = uniform;
        }
        return mixed;
    }

    // For the prototype, we do a simple uniform average of all active predictors.
    // In later phases, this will use logistic mixing with SSE-trained weights.
    
    // First, accumulate probabilities
    for (auto& predictor : m_predictors) {
        ByteDistribution p = predictor->predict(ctx);
        for (u32 i = 0; i < 256; ++i) {
            mixed[static_cast<u8>(i)] += p[static_cast<u8>(i)];
        }
    }

    // Average them
    f32 weight = 1.0f / static_cast<f32>(m_predictors.size());
    for (u32 i = 0; i < 256; ++i) {
        mixed[static_cast<u8>(i)] *= weight;
    }

    // Ensure it's perfectly normalized (floating point math can drift)
    mixed.normalize();

    return mixed;
}

void ContextMixer::update(const PredictorContext& ctx, u8 actual) {
    for (auto& predictor : m_predictors) {
        predictor->update(ctx, actual);
    }
}

void ContextMixer::reset() {
    for (auto& predictor : m_predictors) {
        predictor->reset();
    }
}

} // namespace hypercore
