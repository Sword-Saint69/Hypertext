#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ICompressor.hpp — Abstract compressor interface.
// All compression strategies implement this interface.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
/// Configuration passed to a compressor at construction time.
// ─────────────────────────────────────────────────────────────────────────────
struct CompressorConfig {
    Level level             = Level::Default;
    u32   block_size        = Block::DEFAULT_SIZE;  ///< Target block size in bytes
    bool  use_gpu           = true;                 ///< Enable GPU acceleration if available
    bool  use_grammar       = true;                 ///< Enable grammar compression
    bool  use_context_islands = true;               ///< Enable Context Island detection
    bool  use_neural_predictor = false;             ///< Enable optional neural predictor
    u32   dict_size_global  = 256u * 1024u;        ///< Global dictionary size cap
    u32   thread_count      = 0;                    ///< 0 = auto-detect
};

// ─────────────────────────────────────────────────────────────────────────────
/// Statistics reported after a compression or decompression run.
// ─────────────────────────────────────────────────────────────────────────────
struct CompressionStats {
    u64  original_bytes      = 0;
    u64  compressed_bytes    = 0;
    u32  block_count         = 0;
    f64  compression_ratio   = 0.0;     ///< original / compressed
    f64  bits_per_byte       = 0.0;     ///< bpb = 8 × compressed / original
    f64  compress_time_ms    = 0.0;
    f64  decompress_time_ms  = 0.0;
    f64  compress_mbs        = 0.0;     ///< Throughput in MB/s
    f64  gpu_utilization_pct = 0.0;
    u64  peak_ram_bytes      = 0;

    /// Print a summary to stdout.
    void print() const;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Abstract compression interface.
///
/// A compressor takes a stream of input bytes and produces a compressed
/// byte stream. Decompression takes the compressed stream back to original.
///
/// Implementations:
///   - HyperCoreCompressor  (full CPU+GPU pipeline, Phase 7–9)
///   - CpuCompressor        (CPU-only fallback, Phase 7)
///   - StubCompressor       (identity — for testing the pipeline skeleton)
// ─────────────────────────────────────────────────────────────────────────────
class ICompressor {
public:
    virtual ~ICompressor() = default;

    // ── Core operations ───────────────────────────────────────────────────────

    /// Compress data from `input` file path → `output` file path.
    [[nodiscard]] virtual Result<CompressionStats>
    compress_file(const std::string& input_path,
                  const std::string& output_path) = 0;

    /// Decompress data from `input` file path → `output` file path.
    [[nodiscard]] virtual Result<CompressionStats>
    decompress_file(const std::string& input_path,
                    const std::string& output_path) = 0;

    /// Compress an in-memory block → compressed block.
    [[nodiscard]] virtual Result<Block>
    compress_block(BlockView input) = 0;

    /// Decompress an in-memory block → decompressed block.
    [[nodiscard]] virtual Result<Block>
    decompress_block(BlockView input) = 0;

    // ── Metadata ──────────────────────────────────────────────────────────────

    /// Human-readable name of this compressor implementation.
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Configuration used by this compressor.
    [[nodiscard]] virtual const CompressorConfig& config() const noexcept = 0;
};

} // namespace hypercore
