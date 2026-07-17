#include <gtest/gtest.h>
#include <blake3.h>
#include <string>

TEST(Blake3, BasicHash) {
    // Empty string hash is well-known for BLAKE3
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    
    // "hello world" test
    std::string data = "hello world";
    blake3_hasher_update(&hasher, data.data(), data.size());
    
    uint8_t hash[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, hash, BLAKE3_OUT_LEN);
    
    // The BLAKE3 hash of "hello world" is:
    // d74981efa70a0c880b8d8c1985d075dbcbf679b99a5f9914e5aaf96b831a9e24
    EXPECT_EQ(hash[0], 0xd7);
    EXPECT_EQ(hash[1], 0x49);
    EXPECT_EQ(hash[2], 0x81);
    EXPECT_EQ(hash[3], 0xef);
}
