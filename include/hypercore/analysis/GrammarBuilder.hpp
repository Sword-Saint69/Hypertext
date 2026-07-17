#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// GrammarBuilder.hpp — Builds a Straight-Line Program (SLP) grammar using Re-Pair
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/analysis/Token.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace hypercore {
namespace analysis {

// ─────────────────────────────────────────────────────────────────────────────
// Represents a single grammar rule (S -> AB)
// ─────────────────────────────────────────────────────────────────────────────
struct GrammarRule {
    u32 id;          ///< The new symbol ID (>= 256 for bytes)
    u32 left_child;  ///< Left symbol
    u32 right_child; ///< Right symbol
};

// ─────────────────────────────────────────────────────────────────────────────
// Stage 3: Grammar Builder (CPU Prototype for Re-Pair)
// ─────────────────────────────────────────────────────────────────────────────
class GrammarBuilder {
public:
    GrammarBuilder() = default;

    /// Build a grammar directly from raw bytes.
    /// Initializes sequence where each byte is symbol 0-255.
    void build_from_bytes(ByteSpan block_data);

    /// Get the generated rules
    [[nodiscard]] const std::vector<GrammarRule>& get_rules() const noexcept { return m_rules; }

    /// Get the final compressed sequence of symbols
    [[nodiscard]] const std::vector<u32>& get_compressed_sequence() const noexcept { return m_sequence; }

    /// Clear current state
    void clear();

private:
    std::vector<u32>         m_sequence;
    std::vector<GrammarRule> m_rules;
    u32                      m_next_symbol = 256; // Start new symbols after byte values

    // Internal helper to run the O(N^2) greedy Re-Pair algorithm
    void run_repair();
};

} // namespace analysis
} // namespace hypercore
