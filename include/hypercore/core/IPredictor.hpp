#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// IPredictor.hpp — Abstract predictor interface.
// Predictors estimate the probability of the next symbol given context.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>
#include <array>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
/// A probability estimate for each possible byte value (0–255).
/// Values are in the range [0.0f, 1.0f] and sum to 1.0f.
// ─────────────────────────────────────────────────────────────────────────────
struct ByteDistribution {
    std::array<f32, 256> prob = {};

    /// Returns the probability of byte value `b`.
    [[nodiscard]] f32  operator[](u8 b)  const noexcept { return prob[b]; }
    [[nodiscard]] f32& operator[](u8 b)        noexcept { return prob[b]; }

    /// Validate that probabilities sum to approximately 1.0.
    [[nodiscard]] bool is_valid() const noexcept;

    /// Normalize so probabilities sum to exactly 1.0.
    void normalize() noexcept;
};

// ─────────────────────────────────────────────────────────────────────────────
/// A compressed probability estimate used by the entropy coder.
/// Values are quantized to [0, M] where M = 4096 (2^12).
// ─────────────────────────────────────────────────────────────────────────────
struct QuantizedDistribution {
    static constexpr u32 TABLE_SIZE = 4096u; ///< M = 2^12

    std::array<u32, 256> freq = {};  ///< Quantized frequencies — must sum to TABLE_SIZE

    /// Returns the frequency of byte value `b`.
    [[nodiscard]] u32  operator[](u8 b)  const noexcept { return freq[b]; }
    [[nodiscard]] u32& operator[](u8 b)        noexcept { return freq[b]; }

    /// Build from a ByteDistribution.
    [[nodiscard]] static QuantizedDistribution from(const ByteDistribution& dist) noexcept;

    /// Validate that frequencies sum to TABLE_SIZE.
    [[nodiscard]] bool is_valid() const noexcept;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Context passed to a predictor when asking for a probability estimate.
// ─────────────────────────────────────────────────────────────────────────────
struct PredictorContext {
    ByteSpan  history;       ///< Preceding bytes (up to predictor's order)
    IslandType island;       ///< Current Context Island type
    u32        position;     ///< Position in the current block
};

// ─────────────────────────────────────────────────────────────────────────────
/// Predictor type identifiers — used for adaptive selection.
// ─────────────────────────────────────────────────────────────────────────────
enum class PredictorType : u8 {
    Character   = 0,   ///< Byte n-gram model
    Word        = 1,   ///< Word-boundary-aware context model
    Phrase      = 2,   ///< Phrase repetition model
    Dictionary  = 3,   ///< Dictionary lookup model
    Grammar     = 4,   ///< Grammar rule model
    Structural  = 5,   ///< Bracket/tag nesting model
    Neural      = 6,   ///< Optional lightweight neural model
};

// ─────────────────────────────────────────────────────────────────────────────
/// Abstract predictor interface.
///
/// Each predictor specializes in a different aspect of context modeling.
/// The probability mixer (Stage 4) selects and combines their outputs.
///
/// Planned implementations (Phase 6):
///   - CharacterPredictor   — byte n-gram statistics
///   - WordPredictor        — word-level context
///   - PhrasePredictor      — phrase repetition
///   - DictionaryPredictor  — dictionary-based lookup
///   - GrammarPredictor     — grammar rule predictions
///   - StructuralPredictor  — nesting depth tracking
///   - NeuralPredictor      — RWKV/Mamba-lite (optional, Phase 13)
// ─────────────────────────────────────────────────────────────────────────────
class IPredictor {
public:
    virtual ~IPredictor() = default;

    // ── Core operations ───────────────────────────────────────────────────────

    /// Return a probability distribution for the next byte given context.
    [[nodiscard]] virtual ByteDistribution
    predict(const PredictorContext& ctx) = 0;

    /// Update the model after observing byte `actual` in context `ctx`.
    /// Called after every symbol to allow online adaptation.
    virtual void update(const PredictorContext& ctx, u8 actual) = 0;

    /// Reset the model to its initial state (e.g., between files).
    virtual void reset() noexcept = 0;

    // ── Metadata ──────────────────────────────────────────────────────────────

    [[nodiscard]] virtual PredictorType   type()  const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Maximum context order this predictor uses (in bytes).
    [[nodiscard]] virtual u32 order() const noexcept = 0;

    /// Approximate RAM usage of this predictor's model tables.
    [[nodiscard]] virtual u64 memory_bytes() const noexcept = 0;
};

} // namespace hypercore
