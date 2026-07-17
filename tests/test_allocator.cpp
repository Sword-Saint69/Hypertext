// test_allocator.cpp — Unit tests for PageAlignedAllocator and BumpPool.

#include <gtest/gtest.h>
#include <hypercore/memory/Allocator.hpp>
#include <numeric>
#include <vector>

using namespace hypercore;
using namespace hypercore::memory;

// ─── PageAlignedAllocator ─────────────────────────────────────────────────────

TEST(PageAlignedAllocator, AllocateAndDeallocate) {
    PageAlignedAllocator<u8> alloc;
    u8* ptr = alloc.allocate(1024);
    ASSERT_NE(ptr, nullptr);

    // Verify 64-byte alignment
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0u);

    // Write and read back
    std::iota(ptr, ptr + 1024, u8{0});
    EXPECT_EQ(ptr[0],    0u);
    EXPECT_EQ(ptr[255], 255u);

    alloc.deallocate(ptr, 1024);
}

TEST(PageAlignedAllocator, AlignedVector) {
    AlignedBytes vec(4096, 0xAB);
    EXPECT_EQ(vec.size(), 4096u);

    // Alignment guaranteed by custom allocator
    EXPECT_EQ(reinterpret_cast<uintptr_t>(vec.data()) % 64, 0u);

    for (auto b : vec) EXPECT_EQ(b, 0xABu);
}

TEST(PageAlignedAllocator, EqualityOperator) {
    PageAlignedAllocator<u8>   a1;
    PageAlignedAllocator<u8>   a2;
    PageAlignedAllocator<u32>  a3;

    EXPECT_EQ(a1,  a2);  // All instances of same type are equal (stateless)
    EXPECT_TRUE(a1 == a3);
}

TEST(PageAlignedAllocator, ZeroSizeVector) {
    AlignedBytes vec;
    EXPECT_EQ(vec.size(), 0u);
    EXPECT_TRUE(vec.empty());
}

// ─── BumpPool ─────────────────────────────────────────────────────────────────

TEST(BumpPool, BasicAlloc) {
    BumpPool pool(1024);
    EXPECT_EQ(pool.capacity(), 1024u);
    EXPECT_EQ(pool.used(),     0u);

    void* p = pool.alloc(64, 64);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(pool.used(), 64u);
    EXPECT_EQ(pool.remaining(), 960u);
}

TEST(BumpPool, AlignmentRespected) {
    BumpPool pool(4096);

    // Alloc 1 byte, then 16-byte aligned alloc
    void* a = pool.alloc(1, 1);
    ASSERT_NE(a, nullptr);

    void* b = pool.alloc(16, 16);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(b) % 16, 0u);
}

TEST(BumpPool, MultipleAllocs) {
    BumpPool pool(4096);
    std::vector<void*> ptrs;

    for (int i = 0; i < 10; ++i) {
        void* p = pool.alloc(128, 16);
        ASSERT_NE(p, nullptr);
        ptrs.push_back(p);
    }

    // All pointers should be distinct
    for (std::size_t i = 0; i < ptrs.size(); ++i) {
        for (std::size_t j = i + 1; j < ptrs.size(); ++j) {
            EXPECT_NE(ptrs[i], ptrs[j]);
        }
    }
}

TEST(BumpPool, FullPoolReturnsNull) {
    BumpPool pool(64);
    void* a = pool.alloc(64, 1);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(pool.remaining(), 0u);

    // Next alloc should fail
    void* b = pool.alloc(1, 1);
    EXPECT_EQ(b, nullptr);
}

TEST(BumpPool, ResetReclaims) {
    BumpPool pool(512);
    void* a = pool.alloc(256, 1);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(pool.used(), 256u);

    pool.reset();
    EXPECT_EQ(pool.used(),      0u);
    EXPECT_EQ(pool.remaining(), 512u);

    // Should be able to alloc again
    void* b = pool.alloc(256, 1);
    ASSERT_NE(b, nullptr);
    // After reset, may get the same address
    EXPECT_EQ(a, b);
}

TEST(BumpPool, AllocObject) {
    struct Point { int x; int y; };

    BumpPool pool(1024);
    Point* p = pool.alloc_object<Point>(3, 7);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->x, 3);
    EXPECT_EQ(p->y, 7);
}
