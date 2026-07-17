#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// FileAnalyzer.hpp — Analyzes a file or buffer for encoding and structure
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>
#include <vector>

namespace hypercore {
namespace analysis {

// ─────────────────────────────────────────────────────────────────────────────
// Describes the detected properties of a file or buffer
// ─────────────────────────────────────────────────────────────────────────────
struct FileMetadata {
    size_t     total_bytes = 0;
    bool       is_valid_utf8 = true;
    bool       is_binary = false;
    IslandType global_island_hint = IslandType::Unknown;
};

// ─────────────────────────────────────────────────────────────────────────────
// Stage 0: File/Buffer analyzer to partition and detect encodings
// ─────────────────────────────────────────────────────────────────────────────
class FileAnalyzer {
public:
    FileAnalyzer() = default;

    /// Analyzes a raw byte span (e.g. memory mapped file) and determines metadata
    [[nodiscard]] FileMetadata analyze_buffer(ByteSpan data) const;

    /// Validates if a buffer is valid UTF-8
    [[nodiscard]] static bool is_valid_utf8(ByteSpan data) noexcept;

    /// Very fast check on the first few KB to detect if it's likely binary
    [[nodiscard]] static bool detect_binary(ByteSpan data) noexcept;
};

} // namespace analysis
} // namespace hypercore
