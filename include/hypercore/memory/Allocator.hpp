#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Allocator.hpp — Memory allocators for CPU and GPU-pinned memory.
//
// PageAlignedAllocator<T>  — 64-byte aligned CPU allocator (SIMD safe)
// PinnedAllocator<T>       — CUDA page-locked host allocator (fast DMA)
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/common.hpp>
#include <memory>
#include <new>
#include <limits>
#include <stdexcept>

namespace hypercore {
namespace memory {

// ─────────────────────────────────────────────────────────────────────────────
/// STL-compatible allocator that allocates memory aligned to 64 bytes
/// (cache line size). Safe for SIMD (AVX2 / AVX-512 / NEON) operations.
// ─────────────────────────────────────────────────────────────────────────────
template<typename T>
class PageAlignedAllocator {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    static constexpr std::size_t ALIGNMENT = 64;  ///< Cache-line alignment

    PageAlignedAllocator() noexcept = default;

    template<typename U>
    explicit PageAlignedAllocator(const PageAlignedAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length{};

        const std::size_t bytes = n * sizeof(T);
        void* ptr = ::operator new(bytes, std::align_val_t{ALIGNMENT});
        if (!ptr) throw std::bad_alloc{};
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, [[maybe_unused]] std::size_t n) noexcept {
        ::operator delete(p, std::align_val_t{ALIGNMENT});
    }

    template<typename U>
    bool operator==(const PageAlignedAllocator<U>&) const noexcept { return true; }
    template<typename U>
    bool operator!=(const PageAlignedAllocator<U>&) const noexcept { return false; }
};

/// Aligned vector type — use instead of std::vector for SIMD-operated buffers.
template<typename T>
using AlignedVector = std::vector<T, PageAlignedAllocator<T>>;

using AlignedBytes = AlignedVector<u8>;

// ─────────────────────────────────────────────────────────────────────────────
/// CUDA page-locked (pinned) host memory allocator.
///
/// Pinned memory enables async DMA transfers between CPU and GPU without
/// OS-managed copy — critical for minimizing PCIe transfer latency.
///
/// On non-CUDA builds this falls back to PageAlignedAllocator.
// ─────────────────────────────────────────────────────────────────────────────
template<typename T>
class PinnedAllocator {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    PinnedAllocator() noexcept = default;

    template<typename U>
    explicit PinnedAllocator(const PinnedAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n);
    void deallocate(T* p, std::size_t n) noexcept;

    template<typename U>
    bool operator==(const PinnedAllocator<U>&) const noexcept { return true; }
    template<typename U>
    bool operator!=(const PinnedAllocator<U>&) const noexcept { return false; }
};

/// Pinned vector type — use for CPU↔GPU transfer buffers.
template<typename T>
using PinnedVector = std::vector<T, PinnedAllocator<T>>;

using PinnedBytes = PinnedVector<u8>;

// ─────────────────────────────────────────────────────────────────────────────
/// Simple fixed-size memory pool for zero-allocation hot paths.
///
/// Allocates a large contiguous slab upfront and hands out chunks
/// via a bump pointer. Reset() reclaims all memory at once (no per-item free).
// ─────────────────────────────────────────────────────────────────────────────
class BumpPool : NonCopyable {
public:
    explicit BumpPool(std::size_t capacity_bytes);
    ~BumpPool();

    /// Allocate `n` bytes aligned to `alignment`. Returns nullptr if full.
    [[nodiscard]] void* alloc(std::size_t n, std::size_t alignment = alignof(std::max_align_t)) noexcept;

    /// Allocate a typed object (placement new).
    template<typename T, typename... Args>
    [[nodiscard]] T* alloc_object(Args&&... args) {
        void* mem = alloc(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    /// Reset the pool — all previous allocations become invalid.
    void reset() noexcept { offset_ = 0; }

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t used()     const noexcept { return offset_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return capacity_ - offset_; }

private:
    u8*         base_     = nullptr;
    std::size_t capacity_ = 0;
    std::size_t offset_   = 0;
};

} // namespace memory
} // namespace hypercore
