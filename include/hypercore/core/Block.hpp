#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Block.hpp — The fundamental unit of data flowing through the HyperCore pipeline.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <cassert>
#include <utility>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
/// Metadata attached to every block after preprocessing.
// ─────────────────────────────────────────────────────────────────────────────
struct BlockMeta {
    u64      block_id       = 0;        ///< Monotonically increasing block index
    u64      file_offset    = 0;        ///< Byte offset in the original file
    u32      original_size  = 0;        ///< Uncompressed size in bytes
    u32      compressed_size = 0;       ///< Compressed size in bytes (0 = not yet encoded)
    IslandType dominant_island = IslandType::Unknown; ///< Most common island in this block
    u8       island_count   = 0;        ///< Number of distinct islands
    bool     is_last        = false;    ///< True if this is the final block in the file
};

// ─────────────────────────────────────────────────────────────────────────────
/// A non-owning view into a raw byte buffer — zero-copy block reference.
// ─────────────────────────────────────────────────────────────────────────────
struct BlockView {
    const u8* data   = nullptr;
    u32       size   = 0;
    BlockMeta meta   = {};

    [[nodiscard]] bool         empty()  const noexcept { return size == 0; }
    [[nodiscard]] ByteSpan     span()   const noexcept { return {data, size}; }
    [[nodiscard]] const u8*    begin()  const noexcept { return data; }
    [[nodiscard]] const u8*    end()    const noexcept { return data + size; }
};

// ─────────────────────────────────────────────────────────────────────────────
/// An owning block of raw bytes.
///
/// Used throughout the pipeline to carry data between stages.
/// The buffer is always aligned to 64 bytes (cache line size) for SIMD safety.
// ─────────────────────────────────────────────────────────────────────────────
class Block : NonCopyable {
public:
    /// Default block size: 256 KB. Tuned to balance GPU transfer overhead
    /// and per-block grammar/dictionary overhead.
    static constexpr u32 DEFAULT_SIZE = 256u * 1024u;

    /// Minimum block size: 64 KB (below this, GPU transfer cost dominates).
    static constexpr u32 MIN_SIZE = 64u * 1024u;

    /// Maximum block size: 16 MB.
    static constexpr u32 MAX_SIZE = 16u * 1024u * 1024u;

    /// Cache-line alignment for SIMD operations.
    static constexpr std::align_val_t ALIGNMENT{64};

    // ── Constructors ─────────────────────────────────────────────────────────

    Block() = default;

    /// Allocate a block with the given capacity.
    explicit Block(u32 capacity);

    /// Move constructor.
    Block(Block&& other) noexcept;

    /// Move assignment.
    Block& operator=(Block&& other) noexcept;

    ~Block();

    // ── Factory functions ─────────────────────────────────────────────────────

    /// Create a block by copying data from a span.
    [[nodiscard]] static Block from(ByteSpan data);

    /// Create a block by copying data from a raw pointer.
    [[nodiscard]] static Block from(const u8* data, u32 size);

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] u8*       data()       noexcept { return data_; }
    [[nodiscard]] const u8* data() const noexcept { return data_; }
    [[nodiscard]] u32       size() const noexcept { return size_; }
    [[nodiscard]] u32       capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool      empty() const noexcept { return size_ == 0; }

    [[nodiscard]] BlockMeta&       meta()       noexcept { return meta_; }
    [[nodiscard]] const BlockMeta& meta() const noexcept { return meta_; }

    [[nodiscard]] ByteSpan   span()  const noexcept { return {data_, size_}; }
    [[nodiscard]] BlockView  view()  const noexcept { return {data_, size_, meta_}; }

    [[nodiscard]] u8*       begin()       noexcept { return data_; }
    [[nodiscard]] const u8* begin() const noexcept { return data_; }
    [[nodiscard]] u8*       end()         noexcept { return data_ + size_; }
    [[nodiscard]] const u8* end()   const noexcept { return data_ + size_; }

    // ── Mutation ──────────────────────────────────────────────────────────────

    /// Set the used size (must be ≤ capacity).
    void set_size(u32 s) noexcept {
        assert(s <= capacity_);
        size_ = s;
    }

    /// Resize buffer (reallocates if new capacity > current).
    void resize(u32 new_capacity);

    /// Reset to zero-size (does not free memory).
    void clear() noexcept { size_ = 0; }

private:
    u8*       data_     = nullptr;
    u32       size_     = 0;
    u32       capacity_ = 0;
    BlockMeta meta_     = {};
};

} // namespace hypercore
