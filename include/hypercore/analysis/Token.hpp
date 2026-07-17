#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Token.hpp — Defines basic lexical tokens for the TextAnalyzer
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>

namespace hypercore {
namespace analysis {

// ─────────────────────────────────────────────────────────────────────────────
/// Broad categories of text tokens detected during Phase 3 preprocessing.
// ─────────────────────────────────────────────────────────────────────────────
enum class TokenType : u8 {
    Word,        ///< Contiguous alphabetic characters [a-zA-Z]
    Number,      ///< Contiguous digits [0-9] (ignores floats for simplicity)
    Whitespace,  ///< Spaces, tabs, newlines [\r\n\t ]
    Punctuation, ///< ASCII symbols and punctuation
    Control,     ///< ASCII control characters (< 0x20)
    Unicode,     ///< Multi-byte UTF-8 sequence
    Invalid      ///< Invalid UTF-8 sequence or stray byte
};

[[nodiscard]] inline std::string_view to_string(TokenType t) noexcept {
    switch (t) {
        case TokenType::Word:        return "Word";
        case TokenType::Number:      return "Number";
        case TokenType::Whitespace:  return "Whitespace";
        case TokenType::Punctuation: return "Punctuation";
        case TokenType::Control:     return "Control";
        case TokenType::Unicode:     return "Unicode";
        case TokenType::Invalid:     return "Invalid";
        default:                     return "Unknown";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/// A lightweight reference to a token inside a block.
// ─────────────────────────────────────────────────────────────────────────────
struct Token {
    u32       offset;  ///< Byte offset from the start of the block
    u32       length;  ///< Length of the token in bytes
    TokenType type;

    // Helper for debugging/printing
    [[nodiscard]] std::string_view view(ByteSpan block_span) const noexcept {
        if (offset + length <= block_span.size()) {
            return {reinterpret_cast<const char*>(block_span.data() + offset), length};
        }
        return "";
    }
};

} // namespace analysis
} // namespace hypercore
