// test_smoke.cpp — Sanity checks that the project compiles and links correctly.

#include <gtest/gtest.h>
#include <hypercore/version.hpp>
#include <hypercore/common.hpp>

// ─── Version ─────────────────────────────────────────────────────────────────

TEST(Smoke, VersionConstants) {
    EXPECT_EQ(hypercore::VERSION_MAJOR, 0);
    EXPECT_EQ(hypercore::VERSION_MINOR, 1);
    EXPECT_EQ(hypercore::VERSION_PATCH, 0);
    EXPECT_STREQ(hypercore::VERSION_STRING, "0.1.0");
}

// ─── Error enum ───────────────────────────────────────────────────────────────

TEST(Smoke, ErrorToString) {
    EXPECT_EQ(hypercore::to_string(hypercore::Error::Ok),            "Ok");
    EXPECT_EQ(hypercore::to_string(hypercore::Error::FileNotFound),  "File not found");
    EXPECT_EQ(hypercore::to_string(hypercore::Error::OutOfMemory),   "Out of memory");
    EXPECT_EQ(hypercore::to_string(hypercore::Error::ChecksumMismatch), "Checksum mismatch");
}

// ─── Result<T> ────────────────────────────────────────────────────────────────

TEST(Smoke, ResultOk) {
    hypercore::Result<int> r = 42;
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}

TEST(Smoke, ResultError) {
    hypercore::Result<int> r = std::unexpected(hypercore::Error::InternalError);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), hypercore::Error::InternalError);
}

TEST(Smoke, StatusOk) {
    hypercore::Status s = hypercore::ok();
    EXPECT_TRUE(s.has_value());
}

// ─── IslandType ───────────────────────────────────────────────────────────────

TEST(Smoke, IslandTypeToString) {
    EXPECT_EQ(hypercore::to_string(hypercore::IslandType::SourceCode),      "SourceCode");
    EXPECT_EQ(hypercore::to_string(hypercore::IslandType::Json),            "JSON");
    EXPECT_EQ(hypercore::to_string(hypercore::IslandType::NaturalLanguage), "NaturalLanguage");
    EXPECT_EQ(hypercore::to_string(hypercore::IslandType::Log),             "Log");
}
