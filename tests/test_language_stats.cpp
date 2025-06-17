#include <gtest/gtest.h>
#include "singularity/language_stats.hpp"

namespace {

TEST(LanguageStats, BasicStatsFunctions) {
    singularity::LanguageStats stats;
    
    // Add some files
    stats.add_file("file1.cpp", "C++", 1000);
    stats.add_file("file2.cpp", "C++", 2000);
    stats.add_file("file1.py", "Python", 500);
    stats.add_file("file2.py", "Python", 1500);
    stats.add_file("file1.js", "JavaScript", 750);
    
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
    
    // Check files for each language
    auto cpp_files = stats.get_files_for_language("C++");
    ASSERT_EQ(cpp_files.size(), 2);
    EXPECT_EQ(cpp_files[0], "file1.cpp");
    EXPECT_EQ(cpp_files[1], "file2.cpp");
    
    auto py_files = stats.get_files_for_language("Python");
    ASSERT_EQ(py_files.size(), 2);
    EXPECT_EQ(py_files[0], "file1.py");
    EXPECT_EQ(py_files[1], "file2.py");
    
    auto js_files = stats.get_files_for_language("JavaScript");
    ASSERT_EQ(js_files.size(), 1);
    EXPECT_EQ(js_files[0], "file1.js");
    
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
    EXPECT_TRUE(stats.get_files_for_language("Any").empty());
    
    // Add empty language
    stats.add_file("empty.txt", "Empty", 0);
    EXPECT_EQ(stats.get_language_size("Empty"), 0);
    EXPECT_DOUBLE_EQ(stats.get_language_percentage("Empty"), 0.0);
    EXPECT_EQ(stats.get_files_for_language("Empty").size(), 1);
    
    // Add empty file to non-empty language
    stats.add_file("file.cpp", "C++", 100);
    stats.add_file("empty.cpp", "C++", 0);
    EXPECT_EQ(stats.get_language_size("C++"), 100);
    EXPECT_EQ(stats.get_files_for_language("C++").size(), 2);
}

TEST(LanguageStats, EmptyLanguage) {
    singularity::LanguageStats stats;
    
    // Empty language name should be ignored
    stats.add_file("noext", "", 100);
    EXPECT_EQ(stats.get_total_size(), 0);
    EXPECT_TRUE(stats.get_languages().empty());
}

} // namespace
