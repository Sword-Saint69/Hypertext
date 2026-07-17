// ─────────────────────────────────────────────────────────────────────────────
// Allocator.cpp — Implementations of PinnedAllocator and BumpPool.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/memory/Allocator.hpp>

#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

// Pull in CUDA only if the build has CUDA support.
#if defined(HYPERCORE_CUDA_ENABLED)
#  include <cuda_runtime.h>
#endif

namespace hypercore {
namespace memory {

// ─────────────────────────────────────────────────────────────────────────────
// PinnedAllocator — CUDA page-locked memory, fallback to aligned on CPU builds
// ─────────────────────────────────────────────────────────────────────────────

template<typename T>
T* PinnedAllocator<T>::allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        throw std::bad_array_new_length{};

    const std::size_t bytes = n * sizeof(T);

#if defined(HYPERCORE_CUDA_ENABLED)
    void* ptr = nullptr;
    cudaError_t err = cudaHostAlloc(&ptr, bytes, cudaHostAllocDefault);
    if (err != cudaSuccess || !ptr)
        throw std::bad_alloc{};
    return static_cast<T*>(ptr);
#else
    // CPU-only fallback: 64-byte aligned allocation
    void* ptr = ::operator new(bytes, std::align_val_t{64});
    if (!ptr) throw std::bad_alloc{};
    return static_cast<T*>(ptr);
#endif
}

template<typename T>
void PinnedAllocator<T>::deallocate(T* p, [[maybe_unused]] std::size_t n) noexcept {
    if (!p) return;

#if defined(HYPERCORE_CUDA_ENABLED)
    cudaFreeHost(p);
#else
    ::operator delete(p, std::align_val_t{64});
#endif
}

// Explicit instantiations for common types.
template class PinnedAllocator<uint8_t>;
template class PinnedAllocator<uint32_t>;
template class PinnedAllocator<uint64_t>;
template class PinnedAllocator<float>;

// ─────────────────────────────────────────────────────────────────────────────
// BumpPool
// ─────────────────────────────────────────────────────────────────────────────

BumpPool::BumpPool(std::size_t capacity_bytes)
    : capacity_(capacity_bytes)
    , offset_(0)
{
    base_ = static_cast<u8*>(::operator new(capacity_bytes, std::align_val_t{64}));
    if (!base_) throw std::bad_alloc{};
    std::memset(base_, 0, capacity_bytes);
}

BumpPool::~BumpPool() {
    ::operator delete(base_, std::align_val_t{64});
}

void* BumpPool::alloc(std::size_t n, std::size_t alignment) noexcept {
    // Align offset_ up to `alignment`.
    const std::size_t aligned = (offset_ + alignment - 1) & ~(alignment - 1);
    if (aligned + n > capacity_) return nullptr;

    void* ptr = base_ + aligned;
    offset_   = aligned + n;
    return ptr;
}

} // namespace memory
} // namespace hypercore
