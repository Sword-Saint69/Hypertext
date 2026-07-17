#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// common.hpp — Shared types, aliases, and utilities used across all components.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <expected>
#include <system_error>
#include <functional>

namespace hypercore {

// ─── Fundamental integer aliases ─────────────────────────────────────────────

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

// ─── Byte span aliases ────────────────────────────────────────────────────────

using ByteSpan      = std::span<const u8>;
using MutableSpan   = std::span<u8>;
using ByteVector    = std::vector<u8>;

// ─── Error handling ───────────────────────────────────────────────────────────

/// Error codes for the HyperCore pipeline.
enum class Error : u32 {
    Ok = 0,

    // I/O errors
    FileNotFound,
    FileReadError,
    FileWriteError,
    InvalidPath,

    // Format errors
    InvalidMagic,
    UnsupportedVersion,
    CorruptArchive,
    ChecksumMismatch,

    // Compression errors
    EncodingError,
    DecodingError,
    OutOfMemory,
    BufferTooSmall,

    // GPU errors
    CudaError,
    GpuOutOfMemory,
    GpuNotAvailable,

    // Logic errors
    InvalidArgument,
    NotImplemented,
    InternalError,
};

/// Convert an Error to a human-readable string.
[[nodiscard]] inline std::string_view to_string(Error e) noexcept {
    switch (e) {
        case Error::Ok:                 return "Ok";
        case Error::FileNotFound:       return "File not found";
        case Error::FileReadError:      return "File read error";
        case Error::FileWriteError:     return "File write error";
        case Error::InvalidPath:        return "Invalid path";
        case Error::InvalidMagic:       return "Invalid archive magic bytes";
        case Error::UnsupportedVersion: return "Unsupported archive version";
        case Error::CorruptArchive:     return "Corrupt archive";
        case Error::ChecksumMismatch:   return "Checksum mismatch";
        case Error::EncodingError:      return "Encoding error";
        case Error::DecodingError:      return "Decoding error";
        case Error::OutOfMemory:        return "Out of memory";
        case Error::BufferTooSmall:     return "Buffer too small";
        case Error::CudaError:          return "CUDA error";
        case Error::GpuOutOfMemory:     return "GPU out of memory";
        case Error::GpuNotAvailable:    return "GPU not available";
        case Error::InvalidArgument:    return "Invalid argument";
        case Error::NotImplemented:     return "Not implemented";
        case Error::InternalError:      return "Internal error";
        default:                        return "Unknown error";
    }
}

/// Result type: either a value T or an Error.
template<typename T>
using Result = std::expected<T, Error>;

/// Convenience: a Result with no value (success or error).
using Status = Result<void>;

/// Success sentinel.
[[nodiscard]] inline Status ok() noexcept {
    return {};
}

// ─── Context Island types ─────────────────────────────────────────────────────

/// Semantic region types detected during preprocessing.
enum class IslandType : u8 {
    NaturalLanguage = 0,
    SourceCode      = 1,
    Json            = 2,
    Xml             = 3,
    Csv             = 4,
    Markdown        = 5,
    Log             = 6,
    Binary          = 7,
    Unknown         = 255,
};

[[nodiscard]] inline std::string_view to_string(IslandType t) noexcept {
    switch (t) {
        case IslandType::NaturalLanguage: return "NaturalLanguage";
        case IslandType::SourceCode:      return "SourceCode";
        case IslandType::Json:            return "JSON";
        case IslandType::Xml:             return "XML";
        case IslandType::Csv:             return "CSV";
        case IslandType::Markdown:        return "Markdown";
        case IslandType::Log:             return "Log";
        case IslandType::Binary:          return "Binary";
        default:                          return "Unknown";
    }
}

// ─── Compression level ────────────────────────────────────────────────────────

/// Compression effort level.
enum class Level : u8 {
    Fast    = 1,   ///< Fastest — minimal pattern mining
    Default = 5,   ///< Balanced speed and ratio
    Best    = 9,   ///< Maximum compression ratio
};

// ─── Utility macros ───────────────────────────────────────────────────────────

/// Mark a value as intentionally unused.
template<typename T>
inline void unused(T&&) noexcept {}

/// Non-copyable base class mixin.
struct NonCopyable {
    NonCopyable()                              = default;
    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&)                 = default;
    NonCopyable& operator=(NonCopyable&&)      = default;
};

} // namespace hypercore
