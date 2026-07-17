// test_analysis.cpp — Unit tests for Phase 3 TextAnalyzer

#include <gtest/gtest.h>
#include <hypercore/analysis/TextAnalyzer.hpp>

using namespace hypercore;
using namespace hypercore::analysis;

// ─── Tokenization ─────────────────────────────────────────────────────────────

TEST(TextAnalyzer, TokenizeWordsAndSpaces) {
    const std::string text = "Hello world";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    TextAnalyzer analyzer;
    AnalysisResult res = analyzer.analyze(b.view());
    
    ASSERT_EQ(res.tokens.size(), 3u);
    EXPECT_EQ(res.tokens[0].type, TokenType::Word);
    EXPECT_EQ(res.tokens[0].length, 5u);
    
    EXPECT_EQ(res.tokens[1].type, TokenType::Whitespace);
    EXPECT_EQ(res.tokens[1].length, 1u);
    
    EXPECT_EQ(res.tokens[2].type, TokenType::Word);
    EXPECT_EQ(res.tokens[2].length, 5u);
}

TEST(TextAnalyzer, TokenizeMixed) {
    const std::string text = "Count: 42!";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    TextAnalyzer analyzer;
    AnalysisResult res = analyzer.analyze(b.view());
    
    ASSERT_EQ(res.tokens.size(), 5u);
    EXPECT_EQ(res.tokens[0].type, TokenType::Word);
    EXPECT_EQ(res.tokens[1].type, TokenType::Punctuation); // :
    EXPECT_EQ(res.tokens[2].type, TokenType::Whitespace);
    EXPECT_EQ(res.tokens[3].type, TokenType::Number);
    EXPECT_EQ(res.tokens[4].type, TokenType::Punctuation); // !
}

// ─── Island Detection ─────────────────────────────────────────────────────────

TEST(TextAnalyzer, DetectNaturalLanguage) {
    const std::string text = "This is a simple sentence with normal words.";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    TextAnalyzer analyzer;
    AnalysisResult res = analyzer.analyze(b.view());
    
    ASSERT_FALSE(res.islands.empty());
    EXPECT_EQ(res.islands[0].type, IslandType::NaturalLanguage);
}

TEST(TextAnalyzer, DetectJson) {
    const std::string text = "{ \"key\": [1, 2, 3] }";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    TextAnalyzer analyzer;
    AnalysisResult res = analyzer.analyze(b.view());
    
    ASSERT_FALSE(res.islands.empty());
    EXPECT_EQ(res.islands[0].type, IslandType::Json);
}

TEST(TextAnalyzer, DetectCode) {
    const std::string text = "int main() { return 0; }";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    TextAnalyzer analyzer;
    AnalysisResult res = analyzer.analyze(b.view());
    
    ASSERT_FALSE(res.islands.empty());
    EXPECT_EQ(res.islands[0].type, IslandType::SourceCode);
}
