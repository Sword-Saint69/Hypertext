// ─────────────────────────────────────────────────────────────────────────────
// FileAnalyzer.cpp — Stage 0 implementation for file/buffer analysis
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/analysis/FileAnalyzer.hpp>
#include <algorithm>

namespace hypercore {
namespace analysis {

bool FileAnalyzer::detect_binary(ByteSpan data) noexcept {
    if (data.empty()) return false;

    // Scan first 4KB (or less if file is smaller)
    const size_t check_size = std::min<size_t>(data.size(), 4096);
    size_t null_count = 0;
    size_t control_count = 0;
    size_t printable_count = 0;

    for (size_t i = 0; i < check_size; ++i) {
        u8 c = data[i];
        if (c == 0) {
            null_count++;
        } else if (c < 0x20 && c != '\r' && c != '\n' && c != '\t') {
            control_count++;
        } else if (c >= 0x20 && c < 0x7F) {
            printable_count++;
        }
    }

    // Heuristic: multiple null bytes almost always mean binary.
    if (null_count >= 2) {
        return true;
    }
    
    // Heuristic: If there are many control characters compared to printable ones
    if (printable_count > 0 && (static_cast<double>(control_count) / printable_count) > 0.3) {
        return true;
    }

    return false;
}

bool FileAnalyzer::is_valid_utf8(ByteSpan data) noexcept {
    size_t i = 0;
    while (i < data.size()) {
        u8 c = data[i];
        
        // 1-byte sequence (ASCII)
        if ((c & 0x80) == 0) {
            i++;
            continue;
        }
        
        size_t extra_bytes = 0;
        if ((c & 0xE0) == 0xC0) {
            extra_bytes = 1;
            if (c < 0xC2) return false; // Overlong encoding
        } else if ((c & 0xF0) == 0xE0) {
            extra_bytes = 2;
        } else if ((c & 0xF8) == 0xF0) {
            extra_bytes = 3;
            if (c > 0xF4) return false; // Max unicode code point
        } else {
            return false; // Invalid UTF-8 start byte
        }

        if (i + extra_bytes >= data.size()) {
            return false; // Truncated sequence
        }

        // Check continuation bytes
        for (size_t j = 1; j <= extra_bytes; ++j) {
            if ((data[i + j] & 0xC0) != 0x80) {
                return false;
            }
        }
        
        // Additional checks for specific ranges (overlong/surrogates) could be added here
        
        i += 1 + extra_bytes;
    }
    return true;
}

FileMetadata FileAnalyzer::analyze_buffer(ByteSpan data) const {
    FileMetadata meta;
    meta.total_bytes = data.size();
    
    if (data.empty()) {
        return meta;
    }

    meta.is_binary = detect_binary(data);
    
    if (!meta.is_binary) {
        meta.is_valid_utf8 = is_valid_utf8(data);
    } else {
        meta.is_valid_utf8 = false;
        meta.global_island_hint = IslandType::Binary;
    }

    return meta;
}

} // namespace analysis
} // namespace hypercore
