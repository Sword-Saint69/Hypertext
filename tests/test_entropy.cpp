// test_entropy.cpp — Unit tests for Phase 7 Entropy Coding

#include <gtest/gtest.h>
#include <hypercore/core/BitWriter.hpp>
#include <hypercore/core/RansEncoder.hpp>
#include <hypercore/core/IPredictor.hpp>

using namespace hypercore;

TEST(BitWriter, BasicPacking) {
    BitWriter writer;
    // Write 3 bits: 101 (value 5)
    writer.write_bits(5, 3);
    // Write 5 bits: 11011 (value 27)
    writer.write_bits(27, 5);
    // Total 8 bits. The first byte should now be formed.
    // LSB first:
    // 5 = 101 (binary) -> takes bits 0, 1, 2
    // 27 = 11011 (binary) -> takes bits 3, 4, 5, 6, 7
    // So byte = 5 | (27 << 3) = 5 | 216 = 221
    
    EXPECT_EQ(writer.get_total_bits(), 8);
    writer.flush();
    
    const auto& buf = writer.get_buffer();
    ASSERT_EQ(buf.size(), 1);
    EXPECT_EQ(buf[0], 221);
}

TEST(RansEncoder, EncodeSmokeTest) {
    BitWriter writer;
    RansEncoder encoder(writer);
    
    // Create a dummy distribution (uniform)
    ByteDistribution b_dist;
    for (int i = 0; i < 256; ++i) {
        b_dist[static_cast<u8>(i)] = 1.0f / 256.0f;
    }
    QuantizedDistribution q_dist = QuantizedDistribution::from(b_dist);
    
    // Encode some symbols
    encoder.encode_symbol('A', q_dist);
    encoder.encode_symbol('B', q_dist);
    encoder.encode_symbol('C', q_dist);
    
    encoder.flush();
    
    // Check that we wrote something to the buffer
    const auto& buf = writer.get_buffer();
    EXPECT_GT(buf.size(), 0);
}
