#include "gtest/gtest.h"
#include "singularity/security_recommender.hpp"
#include "singularity/language_stats.hpp"
#include <filesystem>

using namespace singularity;

class SecurityRecommenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        recommender_.initialize();
    }

    // Helper function to create test language stats
    LanguageStats create_test_stats() {
        LanguageStats stats;
        stats.add_file("C++", 1000);
        stats.add_file("C++", 2000);
        stats.add_file("Python", 500);
        return stats;
    }

    SecurityRecommender& recommender_ = SecurityRecommender::instance();
};

TEST_F(SecurityRecommenderTest, GetToolsForLanguage) {
    auto tools = recommender_.get_tools_for_language("C++", SecurityScanType::STATIC_ANALYSIS);
    ASSERT_FALSE(tools.empty());
    
    // Check that we got some of the expected tools from the mapping file
    bool found_clang_analyzer = false;
    bool found_cppcheck = false;
    
    for (const auto& tool : tools) {
        if (tool.name == "Clang Static Analyzer") {
            found_clang_analyzer = true;
        }
        else if (tool.name == "Cppcheck") {
            found_cppcheck = true;
        }
    }
    
    EXPECT_TRUE(found_clang_analyzer);
    EXPECT_TRUE(found_cppcheck);
}

TEST_F(SecurityRecommenderTest, GetToolsForUnsupportedLanguage) {
    auto tools = recommender_.get_tools_for_language("UnknownLanguage", SecurityScanType::STATIC_ANALYSIS);
    EXPECT_TRUE(tools.empty());
}

TEST_F(SecurityRecommenderTest, GetConfigForLanguage) {
    std::string config = recommender_.get_config_for_language("C++", SecurityScanType::STATIC_ANALYSIS);
    EXPECT_FALSE(config.empty());
    EXPECT_TRUE(config.find("Clang Static Analyzer") != std::string::npos);
}

TEST_F(SecurityRecommenderTest, GetGeneralTools) {
    auto tools = recommender_.get_general_tools(SecurityScanType::SECRET_DETECTION);
    ASSERT_FALSE(tools.empty());
    
    // Check that we got some of the expected tools from the mapping file
    bool found_gitleaks = false;
    
    for (const auto& tool : tools) {
        if (tool.name == "GitLeaks") {
            found_gitleaks = true;
        }
    }
    
    EXPECT_TRUE(found_gitleaks);
}

TEST_F(SecurityRecommenderTest, GenerateRecommendations) {
    LanguageStats stats = create_test_stats();
    std::string report = recommender_.generate_recommendations(stats);
    
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("Security Analysis Report") != std::string::npos);
    EXPECT_TRUE(report.find("C++") != std::string::npos);
    EXPECT_TRUE(report.find("Python") != std::string::npos);
}

TEST_F(SecurityRecommenderTest, GenerateGithubWorkflow) {
    LanguageStats stats = create_test_stats();
    // Don't pass a directory to avoid creating files during tests
    std::string workflow = recommender_.generate_github_workflow(stats);
    
    EXPECT_FALSE(workflow.empty());
    EXPECT_TRUE(workflow.find("Security Scan") != std::string::npos);
    EXPECT_TRUE(workflow.find("runs-on") != std::string::npos);
    
    // Test with output directory specified but use a temporary path
    std::string temp_dir = std::filesystem::temp_directory_path().string();
    std::string workflow_summary = recommender_.generate_github_workflow(stats, temp_dir);
    
    EXPECT_FALSE(workflow_summary.empty());
    EXPECT_TRUE(workflow_summary.find("workflow") != std::string::npos);
}
