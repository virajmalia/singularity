#include <gtest/gtest.h>
#include "singularity/language_detector.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

class LanguageDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        temp_dir_ = fs::temp_directory_path() / "singularity_test_XXXXXX";
        fs::create_directories(temp_dir_);
        
        // Initialize test files
        createTestFile("test.cpp", "#include <iostream>\n\nint main() {\n    return 0;\n}\n");
        createTestFile("test.c", "#include <stdio.h>\n\nint main() {\n    return 0;\n}\n");
        createTestFile("test.h", "class TestClass {\npublic:\n    void test();\n};\n");
        createTestFile("test.py", "def main():\n    print('Hello')\n\nif __name__ == '__main__':\n    main()\n");
        createTestFile("test.js", "function test() {\n    console.log('test');\n}\n");
        createTestFile("CMakeLists.txt", "cmake_minimum_required(VERSION 3.10)\nproject(test)\n");
    }

    void TearDown() override {
        // Remove temporary directory and all test files
        fs::remove_all(temp_dir_);
    }

    // Helper to create test files
    void createTestFile(const std::string& filename, const std::string& content) {
        fs::path file_path = temp_dir_ / filename;
        std::ofstream file(file_path.string());
        file << content;
        file.close();
        
        test_files_[filename] = file_path.string();
    }

    fs::path temp_dir_;
    std::map<std::string, std::string> test_files_;
};

TEST_F(LanguageDetectorTest, FileExtensionDetection) {
    auto& detector = singularity::LanguageDetector::instance();
    
    EXPECT_EQ(detector.detect_language(test_files_["test.cpp"]), "C++");
    EXPECT_EQ(detector.detect_language(test_files_["test.c"]), "C");
    EXPECT_EQ(detector.detect_language(test_files_["test.py"]), "Python");
    EXPECT_EQ(detector.detect_language(test_files_["test.js"]), "JavaScript");
}

TEST_F(LanguageDetectorTest, HeaderFileDetection) {
    auto& detector = singularity::LanguageDetector::instance();
    
    // Header file detection (which could be C or C++)
    EXPECT_EQ(detector.detect_language(test_files_["test.h"]), "C++");
}

TEST_F(LanguageDetectorTest, SpecialFilenameDetection) {
    auto& detector = singularity::LanguageDetector::instance();
    
    // CMakeLists.txt should be detected as CMake
    EXPECT_EQ(detector.detect_language(test_files_["CMakeLists.txt"]), "CMake");
}

TEST_F(LanguageDetectorTest, UnknownExtension) {
    auto& detector = singularity::LanguageDetector::instance();
    
    // Create a file with unknown extension
    createTestFile("test.xyz", "Some random content");
    
    // Unknown extension should return empty string
    EXPECT_TRUE(detector.detect_language(test_files_["test.xyz"]).empty());
}

TEST_F(LanguageDetectorTest, FileSize) {
    // Test file size calculation
    createTestFile("size_test.txt", "1234567890");
    
    EXPECT_EQ(singularity::LanguageDetector::get_file_size(test_files_["size_test.txt"]), 10);
    
    // Non-existent file should return 0
    EXPECT_EQ(singularity::LanguageDetector::get_file_size(temp_dir_.string() + "/non_existent.txt"), 0);
}

TEST_F(LanguageDetectorTest, PathPatternMatching) {
    std::vector<std::string> patterns = {
        "*.min.js",
        "*.min.css",
        "node_modules/*",
        ".git/*"
    };
    
    // Test pattern matching
    EXPECT_TRUE(singularity::LanguageDetector::path_matches_patterns("script.min.js", patterns));
    EXPECT_TRUE(singularity::LanguageDetector::path_matches_patterns("style.min.css", patterns));
    EXPECT_TRUE(singularity::LanguageDetector::path_matches_patterns("node_modules/package", patterns));
    EXPECT_TRUE(singularity::LanguageDetector::path_matches_patterns(".git/HEAD", patterns));
    
    // Non-matching paths
    EXPECT_FALSE(singularity::LanguageDetector::path_matches_patterns("script.js", patterns));
    EXPECT_FALSE(singularity::LanguageDetector::path_matches_patterns("style.css", patterns));
    EXPECT_FALSE(singularity::LanguageDetector::path_matches_patterns("modules/package", patterns));
    EXPECT_FALSE(singularity::LanguageDetector::path_matches_patterns("git/HEAD", patterns));
}

} // namespace
