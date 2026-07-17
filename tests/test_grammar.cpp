// test_grammar.cpp — Unit tests for Phase 5 GrammarBuilder

#include <gtest/gtest.h>
#include <hypercore/analysis/GrammarBuilder.hpp>
#include <hypercore/core/Block.hpp>

using namespace hypercore;
using namespace hypercore::analysis;

TEST(GrammarBuilder, BuildFromBytes) {
    // "abcabc" -> S -> "abc"
    const std::string text = "abcabc";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    GrammarBuilder builder;
    builder.build_from_bytes(b.view().span());
    
    auto seq = builder.get_compressed_sequence();
    auto rules = builder.get_rules();
    
    // Original length 6. 
    // Pass 1: "ab" appears twice -> replaced with 256. Sequence: 256 'c' 256 'c'
    // Pass 2: "256 c" appears twice -> replaced with 257. Sequence: 257 257
    EXPECT_EQ(seq.size(), 2u);
    EXPECT_EQ(rules.size(), 2u);
    
    if (rules.size() == 2) {
        EXPECT_EQ(rules[0].id, 256u);
        EXPECT_EQ(rules[0].left_child, static_cast<u32>('a'));
        EXPECT_EQ(rules[0].right_child, static_cast<u32>('b'));
        
        EXPECT_EQ(rules[1].id, 257u);
        EXPECT_EQ(rules[1].left_child, 256u);
        EXPECT_EQ(rules[1].right_child, static_cast<u32>('c'));
        
        EXPECT_EQ(seq[0], 257u);
        EXPECT_EQ(seq[1], 257u);
    }
}

TEST(GrammarBuilder, NoRepeatingPairs) {
    const std::string text = "abcdef";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    GrammarBuilder builder;
    builder.build_from_bytes(b.view().span());
    
    auto seq = builder.get_compressed_sequence();
    auto rules = builder.get_rules();
    
    EXPECT_EQ(seq.size(), 6u);
    EXPECT_EQ(rules.size(), 0u);
}

TEST(GrammarBuilder, ClearResetsState) {
    const std::string text = "abcabc";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    GrammarBuilder builder;
    builder.build_from_bytes(b.view().span());
    EXPECT_FALSE(builder.get_rules().empty());
    
    builder.clear();
    EXPECT_TRUE(builder.get_rules().empty());
    EXPECT_TRUE(builder.get_compressed_sequence().empty());
}
