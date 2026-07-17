// test_pattern_mining.cpp — Unit tests for Phase 4 PatternMiner

#include <gtest/gtest.h>
#include <hypercore/analysis/PatternMiner.hpp>

using namespace hypercore;
using namespace hypercore::analysis;

TEST(PatternMiner, MineNgrams) {
    const std::string text = "the quick brown fox jumps over the quick brown dog";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    TextAnalyzer analyzer;
    AnalysisResult res = analyzer.analyze(b.view());
    
    PatternMiner miner;
    miner.mine_ngrams(b.view().span(), res.tokens);
    
    auto candidates = miner.extract_top_candidates(5);
    
    // We expect "the quick brown" (3-gram) and "quick brown" (2-gram) and "the quick" to be high frequency.
    // Let's just check that we found some candidates.
    ASSERT_FALSE(candidates.empty());
    
    bool found_quick_brown = false;
    for (const auto& c : candidates) {
        if (c.sequence == "quick brown") {
            found_quick_brown = true;
            EXPECT_EQ(c.frequency, 2u); // appears twice
        }
    }
    EXPECT_TRUE(found_quick_brown);
}

TEST(PatternMiner, MinePhrases) {
    // Setup a string with a repeated 8-byte sequence: "repeater"
    const std::string text = "repeater noise noise repeater";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    PatternMiner miner;
    miner.mine_phrases(b.view().span(), 8);
    
    auto candidates = miner.extract_top_candidates(5);
    
    ASSERT_FALSE(candidates.empty());
    
    bool found_repeater = false;
    for (const auto& c : candidates) {
        if (c.sequence == "repeater") {
            found_repeater = true;
            EXPECT_EQ(c.frequency, 2u);
            EXPECT_EQ(c.utility_score, 16u); // 2 * 8
        }
    }
    EXPECT_TRUE(found_repeater);
}

TEST(PatternMiner, ClearResetsState) {
    const std::string text = "testtest";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    PatternMiner miner;
    miner.mine_phrases(b.view().span(), 4);
    EXPECT_FALSE(miner.extract_top_candidates(5).empty());
    
    miner.clear();
    EXPECT_TRUE(miner.extract_top_candidates(5).empty());
}
