// test_archive.cpp — Unit tests for Phase 8 Archive Assembly

#include <gtest/gtest.h>
#include <hypercore/core/ArchiveWriter.hpp>
#include <fstream>
#include <filesystem>
#include <vector>

using namespace hypercore;
namespace fs = std::filesystem;

TEST(ArchiveWriter, BasicWriteAndFinalize) {
    const std::string test_file = "test_archive.htx";
    
    // Clean up before test
    if (fs::exists(test_file)) {
        fs::remove(test_file);
    }
    
    ArchiveWriter writer;
    ASSERT_TRUE(writer.open(test_file).has_value());
    
    // Dummy block
    std::vector<u8> block_data = { 0xDE, 0xAD, 0xBE, 0xEF };
    BlockEntry entry;
    entry.offset = 64;
    entry.compressed_size = 4;
    entry.original_size = 10;
    entry.dominant_island = static_cast<u8>(IslandType::Markdown);
    
    ASSERT_TRUE(writer.write_block(entry, ByteSpan(block_data.data(), block_data.size())).has_value());
    
    ArchiveHeader header;
    header.num_blocks = 1;
    header.original_size = 10;
    header.dict_offset = 0;
    header.grammar_offset = 0;
    
    std::vector<BlockEntry> index = { entry };
    
    ASSERT_TRUE(writer.finalize(header, index).has_value());
    
    // Verify file contents
    std::ifstream in(test_file, std::ios::binary);
    ASSERT_TRUE(in.is_open());
    
    // Check magic bytes
    u8 magic[4];
    in.read(reinterpret_cast<char*>(magic), 4);
    EXPECT_EQ(magic[0], 'H');
    EXPECT_EQ(magic[1], 'T');
    EXPECT_EQ(magic[2], 'X');
    EXPECT_EQ(magic[3], '\x01');
    
    // Skip to block data offset 64
    in.seekg(64, std::ios::beg);
    u8 block_buf[4];
    in.read(reinterpret_cast<char*>(block_buf), 4);
    EXPECT_EQ(block_buf[0], 0xDE);
    EXPECT_EQ(block_buf[1], 0xAD);
    EXPECT_EQ(block_buf[2], 0xBE);
    EXPECT_EQ(block_buf[3], 0xEF);
    
    in.close();
    fs::remove(test_file);
}
