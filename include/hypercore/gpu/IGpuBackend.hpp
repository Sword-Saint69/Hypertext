#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// IGpuBackend.hpp — Abstract GPU backend interface.
// All GPU-accelerated operations go through this interface, enabling a clean
// CPU-only fallback and future AMD HIP portability.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <hypercore/core/Block.hpp>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
/// Information about the GPU device in use.
// ─────────────────────────────────────────────────────────────────────────────
struct GpuDeviceInfo {
    std::string name;
    u64         total_vram_bytes  = 0;
    u64         free_vram_bytes   = 0;
    int         compute_major     = 0;   ///< CUDA compute capability major
    int         compute_minor     = 0;   ///< CUDA compute capability minor
    int         sm_count          = 0;   ///< Number of streaming multiprocessors
    u32         max_threads_per_sm = 0;
    bool        supports_fp16     = false;
    bool        unified_memory    = false;

    [[nodiscard]] bool is_supported() const noexcept {
        // Require CC 7.0+ (Volta and above)
        return compute_major >= 7;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Output of the GPU preprocessing stage (Stage 1).
// ─────────────────────────────────────────────────────────────────────────────
struct GpuPreprocessResult {
    ByteVector    token_types;     ///< Per-token classification
    ByteVector    island_labels;   ///< Per-token Context Island label
    std::vector<u32> token_offsets; ///< Byte offset of each token in the original block
    u32           token_count     = 0;
    bool          is_valid_utf8   = true;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Output of the GPU pattern mining stage (Stage 2).
// ─────────────────────────────────────────────────────────────────────────────
struct GpuMiningResult {
    std::vector<std::pair<u64, u32>> top_ngrams;    ///< (hash, frequency) pairs
    std::vector<std::pair<u64, u32>> top_phrases;   ///< (hash, frequency) pairs
    std::vector<u32>                 freq_table;    ///< Byte frequency histogram [256]
    f64                              entropy_estimate = 0.0; ///< Shannon entropy estimate
};

// ─────────────────────────────────────────────────────────────────────────────
/// Abstract GPU backend interface.
///
/// Implementations:
///   - CudaBackend   — NVIDIA CUDA (Phase 8)
///   - CpuBackend    — CPU fallback (emulates GPU results on CPU, Phase 7)
///
/// All GPU memory is managed internally — callers work with host memory only.
// ─────────────────────────────────────────────────────────────────────────────
class IGpuBackend {
public:
    virtual ~IGpuBackend() = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// Initialize the backend (select device, allocate pools, create streams).
    [[nodiscard]] virtual Status initialize() = 0;

    /// Shutdown and free all GPU resources.
    virtual void shutdown() noexcept = 0;

    // ── Device query ──────────────────────────────────────────────────────────

    [[nodiscard]] virtual const GpuDeviceInfo& device_info() const noexcept = 0;
    [[nodiscard]] virtual bool is_available() const noexcept = 0;

    // ── Pipeline Stage 1: Preprocessing ──────────────────────────────────────

    /// Validate UTF-8, tokenize, classify characters, label Context Islands.
    [[nodiscard]] virtual Result<GpuPreprocessResult>
    preprocess(BlockView input) = 0;

    // ── Pipeline Stage 2: Pattern Mining ─────────────────────────────────────

    /// Compute rolling hashes, n-gram frequencies, phrase candidates,
    /// and entropy estimate for a block.
    [[nodiscard]] virtual Result<GpuMiningResult>
    mine_patterns(BlockView input, const GpuPreprocessResult& preprocess_result) = 0;

    // ── Pipeline Stage 5: Entropy Coding (rANS) ───────────────────────────────

    /// Multi-stream rANS encode.
    /// `freq_table` is a 256-entry quantized frequency table (sums to 4096).
    [[nodiscard]] virtual Result<Block>
    rans_encode(BlockView input, const std::vector<u32>& freq_table) = 0;

    /// Multi-stream rANS decode.
    [[nodiscard]] virtual Result<Block>
    rans_decode(BlockView compressed, u32 original_size,
                const std::vector<u32>& freq_table) = 0;

    // ── Memory stats ──────────────────────────────────────────────────────────

    [[nodiscard]] virtual u64 vram_used_bytes()  const noexcept = 0;
    [[nodiscard]] virtual u64 vram_total_bytes() const noexcept = 0;

    // ── Backend name ──────────────────────────────────────────────────────────

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
};

/// Factory function — returns CudaBackend if CUDA is available, else CpuBackend.
[[nodiscard]] std::unique_ptr<IGpuBackend> create_gpu_backend();

} // namespace hypercore
