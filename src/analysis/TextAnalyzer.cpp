// ─────────────────────────────────────────────────────────────────────────────
// TextAnalyzer.cpp — CPU implementation of Phase 3 text analysis
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/analysis/TextAnalyzer.hpp>
#include <cctype>

namespace hypercore {
namespace analysis {

namespace {

// ASCII character class lookup for fast categorization
enum class CharClass : u8 {
    Alpha,
    Digit,
    Space,
    Punct,
    Ctrl,
    High
};

constexpr CharClass classify(u8 c) noexcept {
    if (c >= 0x80) return CharClass::High;
    if (c >= 'a' && c <= 'z') return CharClass::Alpha;
    if (c >= 'A' && c <= 'Z') return CharClass::Alpha;
    if (c >= '0' && c <= '9') return CharClass::Digit;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return CharClass::Space;
    if (c < 0x20 || c == 0x7F) return CharClass::Ctrl;
    return CharClass::Punct;
}

TokenType map_class_to_token(CharClass cc) noexcept {
    switch (cc) {
        case CharClass::Alpha: return TokenType::Word;
        case CharClass::Digit: return TokenType::Number;
        case CharClass::Space: return TokenType::Whitespace;
        case CharClass::Punct: return TokenType::Punctuation;
        case CharClass::Ctrl:  return TokenType::Control;
        case CharClass::High:  return TokenType::Unicode;
        default:               return TokenType::Invalid;
    }
}

} // namespace

AnalysisResult TextAnalyzer::analyze(BlockView view) const {
    AnalysisResult result;
    if (view.empty()) return result;

    tokenize(view.span(), result);
    detect_islands(view.span(), result);
    return result;
}

void TextAnalyzer::tokenize(ByteSpan data, AnalysisResult& out) const {
    const u32 size = static_cast<u32>(data.size());
    out.tokens.reserve(size / 4); // Heuristic capacity

    u32 alpha_count = 0;
    u32 digit_count = 0;
    u32 ws_count = 0;
    u32 punct_count = 0;
    u32 bracket_count = 0;

    u32 i = 0;
    while (i < size) {
        u8 c = data[i];
        CharClass cc = classify(c);

        if (cc == CharClass::High) {
            // Simplified UTF-8 handling for Phase 3: just emit single byte Unicode tokens
            // Real validation comes in Phase 8 (GPU) or a more robust CPU pass.
            out.tokens.push_back(Token{i, 1, TokenType::Unicode});
            i++;
            continue;
        }

        TokenType tt = map_class_to_token(cc);
        u32 start = i;

        // Consume contiguous characters of the same class (for Word, Number, Whitespace)
        if (tt == TokenType::Word || tt == TokenType::Number || tt == TokenType::Whitespace) {
            while (i < size && classify(data[i]) == cc) {
                i++;
            }
        } else {
            // Punctuation and Control are emitted as single-byte tokens to preserve structure
            if (c == '{' || c == '}' || c == '[' || c == ']' || c == '<' || c == '>') {
                bracket_count++;
            }
            i++;
        }

        u32 len = i - start;
        out.tokens.push_back(Token{start, len, tt});

        // Update stats
        if (tt == TokenType::Word) alpha_count += len;
        else if (tt == TokenType::Number) digit_count += len;
        else if (tt == TokenType::Whitespace) ws_count += len;
        else if (tt == TokenType::Punctuation) punct_count += len;
    }

    if (size > 0) {
        out.alpha_ratio     = static_cast<f64>(alpha_count) / size;
        out.digit_ratio     = static_cast<f64>(digit_count) / size;
        out.ws_ratio        = static_cast<f64>(ws_count) / size;
        out.punct_ratio     = static_cast<f64>(punct_count) / size;
        out.bracket_density = static_cast<f64>(bracket_count) / size;
    }
}

void TextAnalyzer::detect_islands(ByteSpan data, AnalysisResult& out) const {
    // For Phase 3, we implement a simple heuristic:
    // We treat the whole block as one island based on aggregate statistics.
    // Future phases will use sliding windows to find multiple islands per block.

    IslandType primary = IslandType::Unknown;

    if (out.bracket_density > 0.02) {
        // High bracket density implies Code, JSON, or XML
        // Very rough heuristic for Phase 3
        bool has_quotes = false;
        for (const auto& t : out.tokens) {
            if (t.type == TokenType::Punctuation && data[t.offset] == '"') {
                has_quotes = true;
                break;
            }
        }
        primary = has_quotes ? IslandType::Json : IslandType::SourceCode;
    } else if (out.alpha_ratio > 0.6 && out.ws_ratio > 0.1) {
        // Lots of letters and spaces -> likely Natural Language
        primary = IslandType::NaturalLanguage;
    } else {
        // Fallback
        primary = IslandType::Binary;
    }

    out.islands.push_back(ContextIslandRegion{0, static_cast<u32>(data.size()), primary});
}

} // namespace analysis
} // namespace hypercore
