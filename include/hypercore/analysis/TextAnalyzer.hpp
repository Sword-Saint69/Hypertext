#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// TextAnalyzer.hpp — Analyzes a text block for tokens and Context Islands
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>
#include <hypercore/analysis/Token.hpp>
#include <vector>

namespace hypercore {
namespace analysis {

// ─────────────────────────────────────────────────────────────────────────────
/// Represents a contiguous region of text identified as a specific Context Island.
// ─────────────────────────────────────────────────────────────────────────────
struct ContextIslandRegion {
    u32        start_offset; ///< Start byte offset in the block
    u32        end_offset;   ///< End byte offset (exclusive)
    IslandType type;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Output of the TextAnalyzer for a single block.
// ─────────────────────────────────────────────────────────────────────────────
struct AnalysisResult {
    bool                             is_valid_utf8 = true;
    std::vector<Token>               tokens;
    std::vector<ContextIslandRegion> islands;

    // Aggregate statistics
    f64 alpha_ratio     = 0.0;
    f64 digit_ratio     = 0.0;
    f64 ws_ratio        = 0.0;
    f64 punct_ratio     = 0.0;
    f64 bracket_density = 0.0; // Ratio of { } [ ] < > relative to length
};

// ─────────────────────────────────────────────────────────────────────────────
/// CPU-based text analyzer (Stage 0/1 implementation).
/// Will be superseded by IGpuBackend::preprocess in Phase 8 for CUDA builds.
// ─────────────────────────────────────────────────────────────────────────────
class TextAnalyzer {
public:
    TextAnalyzer() = default;

    /// Analyze a block of data, extracting tokens and identifying islands.
    [[nodiscard]] AnalysisResult analyze(BlockView view) const;

private:
    /// Fast tokenizer pass.
    void tokenize(ByteSpan data, AnalysisResult& out) const;

    /// Heuristic pass to group tokens into Context Islands.
    void detect_islands(ByteSpan data, AnalysisResult& out) const;
};

} // namespace analysis
} // namespace hypercore
