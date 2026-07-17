// ─────────────────────────────────────────────────────────────────────────────
// GrammarBuilder.cpp — CPU implementation of Phase 5 grammar building
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/analysis/GrammarBuilder.hpp>
#include <unordered_map>
#include <algorithm>

namespace hypercore {
namespace analysis {

// Used to hash pairs in our frequency map
struct PairHash {
    std::size_t operator()(const std::pair<u32, u32>& p) const noexcept {
        return (static_cast<std::size_t>(p.first) << 32) | p.second;
    }
};

void GrammarBuilder::clear() {
    m_sequence.clear();
    m_rules.clear();
    m_next_symbol = 256;
}

void GrammarBuilder::build_from_bytes(ByteSpan block_data) {
    clear();
    
    // Initialize sequence with raw bytes
    m_sequence.reserve(block_data.size());
    for (size_t i = 0; i < block_data.size(); ++i) {
        m_sequence.push_back(block_data.data()[i]);
    }

    run_repair();
}

void GrammarBuilder::run_repair() {
    // Basic O(N^2) Re-Pair implementation for the CPU prototype.
    // 1. Find most frequent pair.
    // 2. If frequency <= 1, stop.
    // 3. Replace non-overlapping occurrences with new symbol.
    // 4. Repeat.

    while (m_sequence.size() >= 2) {
        std::unordered_map<std::pair<u32, u32>, u32, PairHash> pair_counts;
        
        // Count frequencies of all adjacent pairs
        for (size_t i = 0; i < m_sequence.size() - 1; ++i) {
            std::pair<u32, u32> p = {m_sequence[i], m_sequence[i+1]};
            pair_counts[p]++;
        }

        // Find the most frequent pair
        u32 max_count = 0;
        std::pair<u32, u32> best_pair = {0, 0};

        for (const auto& [p, count] : pair_counts) {
            if (count > max_count) {
                max_count = count;
                best_pair = p;
            }
        }

        // Stop if no pair appears more than once
        if (max_count <= 1) {
            break;
        }

        // Create new rule
        u32 new_symbol = m_next_symbol++;
        m_rules.push_back({new_symbol, best_pair.first, best_pair.second});

        // Replace non-overlapping occurrences in the sequence
        std::vector<u32> next_sequence;
        next_sequence.reserve(m_sequence.size());

        for (size_t i = 0; i < m_sequence.size(); ++i) {
            if (i < m_sequence.size() - 1 && m_sequence[i] == best_pair.first && m_sequence[i+1] == best_pair.second) {
                next_sequence.push_back(new_symbol);
                i++; // Skip the next symbol as it was merged
            } else {
                next_sequence.push_back(m_sequence[i]);
            }
        }

        m_sequence = std::move(next_sequence);
    }
}

} // namespace analysis
} // namespace hypercore
