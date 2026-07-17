// test_file_analysis.cpp — Unit tests for Phase 3 FileAnalyzer

#include <gtest/gtest.h>
#include <hypercore/analysis/FileAnalyzer.hpp>

using namespace hypercore;
using namespace hypercore::analysis;

// ─── UTF-8 Validation ────────────────────────────────────────────────────────

TEST(FileAnalyzer, Utf8ValidAscii) {
    const std::string text = "Hello world 123 !@#";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    EXPECT_TRUE(FileAnalyzer::is_valid_utf8(b.view().span()));
}

TEST(FileAnalyzer, Utf8ValidMultiByte) {
    const std::string text = "Hello 🌍 world! \xE2\x82\xAC"; // Earth globe and Euro sign
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    EXPECT_TRUE(FileAnalyzer::is_valid_utf8(b.view().span()));
}

TEST(FileAnalyzer, Utf8InvalidContinuation) {
    const u8 data[] = { 'H', 'i', 0xC2, 0x20 }; // 0xC2 must be followed by 0x80-0xBF
    EXPECT_FALSE(FileAnalyzer::is_valid_utf8(ByteSpan(data, 4)));
}

TEST(FileAnalyzer, Utf8InvalidStart) {
    const u8 data[] = { 'H', 'i', 0xFF, ' ' }; // 0xFF is invalid in UTF-8
    EXPECT_FALSE(FileAnalyzer::is_valid_utf8(ByteSpan(data, 4)));
}

// ─── Binary Detection ────────────────────────────────────────────────────────

TEST(FileAnalyzer, DetectTextAsNotBinary) {
    const std::string text = "This is a normal text string with a newline\nand a tab\t.";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    EXPECT_FALSE(FileAnalyzer::detect_binary(b.view().span()));
}

TEST(FileAnalyzer, DetectNullsAsBinary) {
    const u8 data[] = { 'A', 'B', 0x00, 'C', 'D', 0x00, 'E' };
    EXPECT_TRUE(FileAnalyzer::detect_binary(ByteSpan(data, 7)));
}

TEST(FileAnalyzer, DetectHighControlRatioAsBinary) {
    const u8 data[] = { 'A', 0x01, 0x02, 0x03, 'B', 0x04, 0x05, 0x06, 'C' };
    EXPECT_TRUE(FileAnalyzer::detect_binary(ByteSpan(data, 9)));
}

// ─── Analyze Buffer ──────────────────────────────────────────────────────────

TEST(FileAnalyzer, AnalyzeValidText) {
    const std::string text = "Some sample text content here.";
    Block b = Block::from(reinterpret_cast<const u8*>(text.data()), static_cast<u32>(text.size()));
    
    FileAnalyzer analyzer;
    FileMetadata meta = analyzer.analyze_buffer(b.view().span());
    
    EXPECT_EQ(meta.total_bytes, text.size());
    EXPECT_FALSE(meta.is_binary);
    EXPECT_TRUE(meta.is_valid_utf8);
    EXPECT_EQ(meta.global_island_hint, IslandType::Unknown); // Doesn't set hint for text yet
}

TEST(FileAnalyzer, AnalyzeBinaryFile) {
    const u8 data[] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n', 0x00, 0x00, 0x00, 0x0D, 'I', 'H', 'D', 'R' };
    
    FileAnalyzer analyzer;
    FileMetadata meta = analyzer.analyze_buffer(ByteSpan(data, sizeof(data)));
    
    EXPECT_EQ(meta.total_bytes, sizeof(data));
    EXPECT_TRUE(meta.is_binary);
    EXPECT_FALSE(meta.is_valid_utf8);
    EXPECT_EQ(meta.global_island_hint, IslandType::Binary);
}
