// test_block.cpp — Unit tests for Block and BlockView.

#include <gtest/gtest.h>
#include <hypercore/core/Block.hpp>
#include <algorithm>
#include <numeric>

using namespace hypercore;

// ─── Default construction ─────────────────────────────────────────────────────

TEST(Block, DefaultConstruct) {
    Block b;
    EXPECT_EQ(b.data(),     nullptr);
    EXPECT_EQ(b.size(),     0u);
    EXPECT_EQ(b.capacity(), 0u);
    EXPECT_TRUE(b.empty());
}

// ─── Capacity construction ────────────────────────────────────────────────────

TEST(Block, CapacityConstruct) {
    Block b(1024u);
    EXPECT_NE(b.data(),     nullptr);
    EXPECT_EQ(b.size(),     0u);
    EXPECT_EQ(b.capacity(), 1024u);
    EXPECT_TRUE(b.empty());

    // Verify pointer is 64-byte aligned
    EXPECT_EQ(reinterpret_cast<uintptr_t>(b.data()) % 64, 0u);
}

// ─── Factory: from ByteSpan ───────────────────────────────────────────────────

TEST(Block, FromSpan) {
    const std::vector<u8> data = {1, 2, 3, 4, 5};
    Block b = Block::from(ByteSpan{data.data(), static_cast<u32>(data.size())});

    EXPECT_EQ(b.size(), 5u);
    EXPECT_FALSE(b.empty());
    EXPECT_EQ(std::vector<u8>(b.begin(), b.end()), data);

    // Pointer alignment
    EXPECT_EQ(reinterpret_cast<uintptr_t>(b.data()) % 64, 0u);
}

// ─── Factory: from raw pointer ────────────────────────────────────────────────

TEST(Block, FromRawPointer) {
    const u8 raw[] = {0xAA, 0xBB, 0xCC};
    Block b = Block::from(raw, 3u);

    ASSERT_EQ(b.size(), 3u);
    EXPECT_EQ(b.data()[0], 0xAAu);
    EXPECT_EQ(b.data()[1], 0xBBu);
    EXPECT_EQ(b.data()[2], 0xCCu);
}

// ─── Move semantics ───────────────────────────────────────────────────────────

TEST(Block, MoveConstruct) {
    Block a = Block::from(ByteSpan{});
    Block a2(1024u);
    a2.set_size(512u);
    const u8* old_ptr = a2.data();

    Block b = std::move(a2);
    EXPECT_EQ(b.data(),     old_ptr);
    EXPECT_EQ(b.size(),     512u);
    EXPECT_EQ(b.capacity(), 1024u);

    // a2 should be empty after move
    EXPECT_EQ(a2.data(),     nullptr);
    EXPECT_EQ(a2.size(),     0u);
    EXPECT_EQ(a2.capacity(), 0u);
}

TEST(Block, MoveAssign) {
    Block a(512u);
    a.set_size(256u);
    const u8* old_ptr = a.data();

    Block b(256u);
    b = std::move(a);

    EXPECT_EQ(b.data(),     old_ptr);
    EXPECT_EQ(b.size(),     256u);
    EXPECT_EQ(b.capacity(), 512u);
}

// ─── set_size ─────────────────────────────────────────────────────────────────

TEST(Block, SetSize) {
    Block b(1024u);
    b.set_size(512u);
    EXPECT_EQ(b.size(), 512u);
    EXPECT_FALSE(b.empty());
}

// ─── clear ────────────────────────────────────────────────────────────────────

TEST(Block, Clear) {
    Block b(128u);
    b.set_size(64u);
    EXPECT_FALSE(b.empty());

    b.clear();
    EXPECT_EQ(b.size(),     0u);
    EXPECT_EQ(b.capacity(), 128u);  // capacity unchanged
    EXPECT_TRUE(b.empty());
}

// ─── resize ───────────────────────────────────────────────────────────────────

TEST(Block, ResizeGrow) {
    const u8 pattern[] = {1, 2, 3, 4};
    Block b = Block::from(pattern, 4u);

    b.resize(4096u);
    EXPECT_EQ(b.capacity(), 4096u);
    EXPECT_EQ(b.size(),     4u);
    // Data preserved after resize
    EXPECT_EQ(b.data()[0], 1u);
    EXPECT_EQ(b.data()[3], 4u);
    // New pointer still aligned
    EXPECT_EQ(reinterpret_cast<uintptr_t>(b.data()) % 64, 0u);
}

// ─── BlockView ────────────────────────────────────────────────────────────────

TEST(BlockView, FromBlock) {
    const u8 data[] = {10, 20, 30};
    Block b = Block::from(data, 3u);
    BlockView v = b.view();

    EXPECT_EQ(v.size, 3u);
    EXPECT_EQ(v.data[0], 10u);
    EXPECT_EQ(v.data[1], 20u);
    EXPECT_EQ(v.data[2], 30u);
    EXPECT_FALSE(v.empty());
}

TEST(BlockView, SpanConversion) {
    const u8 data[] = {5, 6, 7, 8};
    Block b = Block::from(data, 4u);
    BlockView v = b.view();
    ByteSpan s = v.span();

    EXPECT_EQ(s.size(),  4u);
    EXPECT_EQ(s[0],      5u);
    EXPECT_EQ(s[3],      8u);
}

// ─── Constants ────────────────────────────────────────────────────────────────

TEST(Block, Constants) {
    EXPECT_EQ(Block::DEFAULT_SIZE, 256u * 1024u);
    EXPECT_EQ(Block::MIN_SIZE,      64u * 1024u);
    EXPECT_EQ(Block::MAX_SIZE,      16u * 1024u * 1024u);
    EXPECT_GE(Block::DEFAULT_SIZE,  Block::MIN_SIZE);
    EXPECT_LE(Block::DEFAULT_SIZE,  Block::MAX_SIZE);
}

// ─── Large block ─────────────────────────────────────────────────────────────

TEST(Block, LargeBlock) {
    constexpr u32 SIZE = 4u * 1024u * 1024u;  // 4 MB
    Block b(SIZE);
    EXPECT_EQ(b.capacity(), SIZE);

    // Fill with a pattern and verify
    std::iota(b.data(), b.data() + SIZE, u8{0});
    b.set_size(SIZE);
    EXPECT_EQ(b.size(), SIZE);
    EXPECT_EQ(b.data()[0],      0u);
    EXPECT_EQ(b.data()[SIZE-1], static_cast<u8>((SIZE - 1) % 256));
}
