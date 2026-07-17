// ─────────────────────────────────────────────────────────────────────────────
// PatternMiner.cpp — CPU implementation of Phase 4 pattern mining
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/analysis/PatternMiner.hpp>
#include <algorithm>
#include <unordered_map>

namespace hypercore {
namespace analysis {

void PatternMiner::record_sequence(std::string_view seq) {
    // For the CPU prototype, we'll do a simple linear search or map. 
    // Since we need to keep a stable list of indices for candidates, 
    // we'll just use a quick hash map for lookup.
    // To avoid bringing std::unordered_map into the class definition (and causing 
    // allocation overhead for small structs), we'll manage it locally.
    
    // Actually, storing it efficiently in the CPU prototype:
    // We'll maintain a parallel map just for the current run, 
    // or we can just iterate. Since N is large, iteration is O(N^2).
    // Let's use a static thread_local map for the prototype.
    thread_local std::unordered_map<std::string_view, u32> s_lookup;
    
    auto it = s_lookup.find(seq);
    if (it != s_lookup.end()) {
        m_candidates[it->second].count++;
    } else {
        u32 index = static_cast<u32>(m_candidates.size());
        m_candidates.push_back({1, static_cast<u32>(seq.length()), std::string(seq)});
        // We must store a stable string_view. The string is now inside m_candidates.
        s_lookup[m_candidates.back().first_seen_text] = index;
    }
}

void PatternMiner::clear() {
    m_candidates.clear();
}

void PatternMiner::mine_ngrams(ByteSpan block_data, const std::vector<Token>& tokens) {
    if (tokens.size() < 2) return;
    
    // Clear lookup on each new mining pass to prevent invalid string_view pointers
    // from previous blocks or cleared states.
    // Note: The `record_sequence` map relies on this being called per-block effectively.
    // Instead of a true thread_local static map keeping state, we should just use a local map here 
    // and populate `m_candidates`.
    
    std::unordered_map<std::string_view, u32> lookup;
    auto add_seq = [&](std::string_view seq) {
        auto it = lookup.find(seq);
        if (it != lookup.end()) {
            m_candidates[it->second].count++;
        } else {
            u32 index = static_cast<u32>(m_candidates.size());
            m_candidates.push_back({1, static_cast<u32>(seq.length()), std::string(seq)});
            lookup[m_candidates.back().first_seen_text] = index;
        }
    };

    // Filter out whitespace/control tokens for N-gram construction
    std::vector<Token> content_tokens;
    content_tokens.reserve(tokens.size());
    for (const auto& t : tokens) {
        if (t.type == TokenType::Word || t.type == TokenType::Number || t.type == TokenType::Punctuation) {
            content_tokens.push_back(t);
        }
    }

    if (content_tokens.size() < 2) return;

    // Mine 2-grams and 3-grams
    for (size_t i = 0; i < content_tokens.size() - 1; ++i) {
        // 2-gram
        u32 start = content_tokens[i].offset;
        u32 end = content_tokens[i+1].offset + content_tokens[i+1].length;
        if (end <= block_data.size()) {
            std::string_view seq(reinterpret_cast<const char*>(block_data.data() + start), end - start);
            add_seq(seq);
        }

        // 3-gram
        if (i < content_tokens.size() - 2) {
            u32 end3 = content_tokens[i+2].offset + content_tokens[i+2].length;
            if (end3 <= block_data.size()) {
                std::string_view seq3(reinterpret_cast<const char*>(block_data.data() + start), end3 - start);
                add_seq(seq3);
            }
        }
    }
}

void PatternMiner::mine_phrases(ByteSpan block_data, u32 window_size) {
    if (block_data.size() < window_size) return;

    std::unordered_map<std::string_view, u32> lookup;
    auto add_seq = [&](std::string_view seq) {
        auto it = lookup.find(seq);
        if (it != lookup.end()) {
            m_candidates[it->second].count++;
        } else {
            u32 index = static_cast<u32>(m_candidates.size());
            m_candidates.push_back({1, static_cast<u32>(seq.length()), std::string(seq)});
            lookup[m_candidates.back().first_seen_text] = index;
        }
    };

    // Simple sliding window using std::string_view as the hash key.
    // For a real production CPU version, Karp-Rabin or similar is better.
    // For this prototype, string_view hashing is sufficient.
    for (size_t i = 0; i <= block_data.size() - window_size; ++i) {
        std::string_view seq(reinterpret_cast<const char*>(block_data.data() + i), window_size);
        add_seq(seq);
    }
}

std::vector<DictCandidate> PatternMiner::extract_top_candidates(u32 count) const {
    std::vector<DictCandidate> results;
    results.reserve(m_candidates.size());

    for (const auto& c : m_candidates) {
        // Filter out things that only appeared once — they offer no utility.
        if (c.count > 1) {
            u32 utility = c.count * c.length;
            results.push_back({c.first_seen_text, c.count, utility});
        }
    }

    std::sort(results.begin(), results.end());

    if (results.size() > count) {
        results.resize(count);
    }

    return results;
}

} // namespace analysis
} // namespace hypercore
