#include <gtest/gtest.h>
#include "singularity/formatter.hpp"
#include "singularity/language_stats.hpp"
#include <regex>

namespace {

// Helper function to create test stats
singularity::LanguageStats create_test_stats() {
    singularity::LanguageStats stats;
    stats.add_file("file1.cpp", "C++", 3000);
    stats.add_file("file1.py", "Python", 2000);
    stats.add_file("file1.js", "JavaScript", 1000);
    return stats;
}

TEST(FormatterTest, TextFormatter) {
    singularity::TextFormatter formatter;
    auto stats = create_test_stats();
    
    std::string formatted = formatter.format(stats);
    
    // Check that the output contains expected sections
    EXPECT_TRUE(formatted.find("Languages detected") != std::string::npos);
    
    // Check that all languages are present
    EXPECT_TRUE(formatted.find("C++") != std::string::npos);
    EXPECT_TRUE(formatted.find("Python") != std::string::npos);
    EXPECT_TRUE(formatted.find("JavaScript") != std::string::npos);
    
    // Check total
    EXPECT_TRUE(formatted.find("Total languages detected: 3") != std::string::npos);
}

} // namespace
