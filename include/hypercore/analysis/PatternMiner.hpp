#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// PatternMiner.hpp — Mines n-grams and byte phrases to build dictionaries
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/analysis/Token.hpp>
#include <hypercore/analysis/TextAnalyzer.hpp>
#include <string_view>
#include <vector>
#include <string>

namespace hypercore {
namespace analysis {

// ─────────────────────────────────────────────────────────────────────────────
// Represents a potential dictionary entry
// ─────────────────────────────────────────────────────────────────────────────
struct DictCandidate {
    std::string sequence;      ///< The actual bytes/text
    u32         frequency;     ///< How many times it appeared
    u32         utility_score; ///< frequency * length (approximate bit savings)

    // Used for sorting by utility score (descending)
    bool operator<(const DictCandidate& other) const noexcept {
        if (utility_score != other.utility_score)
            return utility_score > other.utility_score;
        // Tie breaker for determinism
        if (frequency != other.frequency)
            return frequency > other.frequency;
        return sequence < other.sequence;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Stage 2: Pattern Miner (CPU Prototype)
// ─────────────────────────────────────────────────────────────────────────────
class PatternMiner {
public:
    PatternMiner() = default;

    /// Mine 2-grams and 3-grams from the token stream (ignores Whitespace for grouping)
    void mine_ngrams(ByteSpan block_data, const std::vector<Token>& tokens);

    /// Mine byte sequences using a simple rolling hash (e.g. for binary/unstructured data)
    /// @param window_size Length of the repeating sequence to look for (default 8)
    void mine_phrases(ByteSpan block_data, u32 window_size = 8);

    /// Extract the top N candidates based on their utility score
    [[nodiscard]] std::vector<DictCandidate> extract_top_candidates(u32 count) const;

    /// Clear current state
    void clear();

private:
    // Internal struct to hold counts before extraction
    struct HashCount {
        u32 count = 0;
        u32 length = 0;
        std::string first_seen_text; // Storing a copy here simplifies the prototype
    };

    // We use a simple hash map for the CPU prototype.
    // In Phase 8, this becomes a GPU-side hash table (cuCollections).
    std::vector<HashCount> m_candidates;
    
    // Internal helper to insert or update a sequence
    void record_sequence(std::string_view seq);
};

} // namespace analysis
} // namespace hypercore
