// ─────────────────────────────────────────────────────────────────────────────
// Block.cpp — Implementation of the Block owning buffer type.
// ─────────────────────────────────────────────────────────────────────────────

#include <hypercore/core/Block.hpp>

#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <utility>

namespace hypercore {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

[[nodiscard]] u8* alloc_aligned(u32 capacity) {
    if (capacity == 0) return nullptr;
    void* ptr = ::operator new(capacity, Block::ALIGNMENT);
    if (!ptr) throw std::bad_alloc{};
    return static_cast<u8*>(ptr);
}

void free_aligned(u8* ptr) noexcept {
    if (ptr) ::operator delete(ptr, Block::ALIGNMENT);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Block constructors
// ─────────────────────────────────────────────────────────────────────────────

Block::Block(u32 capacity)
    : data_(alloc_aligned(capacity))
    , size_(0)
    , capacity_(capacity)
{}

Block::Block(Block&& other) noexcept
    : data_(other.data_)
    , size_(other.size_)
    , capacity_(other.capacity_)
    , meta_(other.meta_)
{
    other.data_     = nullptr;
    other.size_     = 0;
    other.capacity_ = 0;
    other.meta_     = {};
}

Block& Block::operator=(Block&& other) noexcept {
    if (this != &other) {
        free_aligned(data_);
        data_     = other.data_;
        size_     = other.size_;
        capacity_ = other.capacity_;
        meta_     = other.meta_;

        other.data_     = nullptr;
        other.size_     = 0;
        other.capacity_ = 0;
        other.meta_     = {};
    }
    return *this;
}

Block::~Block() {
    free_aligned(data_);
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory functions
// ─────────────────────────────────────────────────────────────────────────────

Block Block::from(ByteSpan data) {
    Block b(static_cast<u32>(data.size()));
    if (!data.empty()) {
        std::memcpy(b.data_, data.data(), data.size());
        b.size_ = static_cast<u32>(data.size());
    }
    return b;
}

Block Block::from(const u8* data, u32 size) {
    return Block::from(ByteSpan{data, size});
}

// ─────────────────────────────────────────────────────────────────────────────
// resize
// ─────────────────────────────────────────────────────────────────────────────

void Block::resize(u32 new_capacity) {
    if (new_capacity <= capacity_) {
        // Shrink: just update size if needed
        if (size_ > new_capacity) size_ = new_capacity;
        return;
    }

    u8* new_data = alloc_aligned(new_capacity);
    if (size_ > 0) {
        std::memcpy(new_data, data_, size_);
    }
    free_aligned(data_);
    data_     = new_data;
    capacity_ = new_capacity;
}

} // namespace hypercore
