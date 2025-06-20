#include <gtest/gtest.h>
#include "singularity/language_stats.hpp"

namespace {

TEST(LanguageStats, BasicStatsFunctions) {
    singularity::LanguageStats stats;

    // Add some files
    stats.add_file("C++", 1000);
    stats.add_file("C++", 2000);
    stats.add_file("Python", 500);
    stats.add_file("Python", 1500);
    stats.add_file("JavaScript", 750);

    // Check language sizes
    EXPECT_EQ(stats.get_language_size("C++"), 3000);
    EXPECT_EQ(stats.get_language_size("Python"), 2000);
    EXPECT_EQ(stats.get_language_size("JavaScript"), 750);
    EXPECT_EQ(stats.get_language_size("Ruby"), 0);  // Non-existent language

    // Check total size
    EXPECT_EQ(stats.get_total_size(), 5750);

    // Check percentages
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("C++"), 52.173913043478265);
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("Python"), 34.782608695652172);
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("JavaScript"), 13.043478260869565);
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("Ruby"), 0.0);  // Non-existent language

    // Check languages list
    auto languages = stats.get_languages();
    ASSERT_EQ(languages.size(), 3);

    // Languages should be sorted by size (descending)
    EXPECT_EQ(languages[0], "C++");
    EXPECT_EQ(languages[1], "Python");
    EXPECT_EQ(languages[2], "JavaScript");

    // Check clear
    stats.clear();
    EXPECT_EQ(stats.get_total_size(), 0);
    EXPECT_TRUE(stats.get_languages().empty());
}

TEST(LanguageStats, ZeroSizeEdgeCases) {
    singularity::LanguageStats stats;

    // Test with empty stats
    EXPECT_EQ(stats.get_total_size(), 0);
    EXPECT_EQ(stats.get_language_size("Any"), 0);
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("Any"), 0.0);
    EXPECT_TRUE(stats.get_languages().empty());

    // Add empty language
    stats.add_file("Empty", 0);
    EXPECT_EQ(stats.get_language_size("Empty"), 0);
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("Empty"), 0.0);

    // Add empty file to non-empty language
    stats.add_file("C++", 100);
    stats.add_file("C++", 0);
    EXPECT_EQ(stats.get_language_size("C++"), 100);
}

TEST(LanguageStats, EmptyLanguage) {
    singularity::LanguageStats stats;

    // Empty language name should be ignored
    stats.add_file("", 100);
    EXPECT_EQ(stats.get_total_size(), 0);
    EXPECT_TRUE(stats.get_languages().empty());
}

} // namespace
