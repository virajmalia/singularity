#include <gtest/gtest.h>
#include "singularity/repo_analyzer.hpp"
#include "singularity/analyzers.hpp"
#include <memory>

namespace {

// Mock of RepoAnalyzer for testing
class MockRepoAnalyzer : public singularity::RepoAnalyzer {
public:
    singularity::LanguageStats analyze() override {
        if (fail_) {
            throw std::runtime_error("Mock failure");
        }
        
        singularity::LanguageStats stats;
        stats.add_file("mock_file.cpp", "C++", 1000);
        stats.add_file("mock_file.py", "Python", 500);
        return stats;
    }
    
    void set_fail(bool fail) { fail_ = fail; }
    
private:
    bool fail_ = false;
};

TEST(RepoAnalyzerTest, ProgressCallback) {
    MockRepoAnalyzer analyzer;
    
    bool callback_called = false;
    int last_percentage = -1;
    std::string last_message;
    
    analyzer.set_progress_callback([&](int percentage, const std::string& message) {
        callback_called = true;
        last_percentage = percentage;
        last_message = message;
    });
    
    // Call report_progress through a test method
    // Since report_progress is protected, we need a custom test method
    class TestableAnalyzer : public MockRepoAnalyzer {
    public:
        void test_report_progress(int percentage, const std::string& message) {
            report_progress(percentage, message);
        }
    };
    
    TestableAnalyzer testable;
    testable.set_progress_callback([&](int percentage, const std::string& message) {
        callback_called = true;
        last_percentage = percentage;
        last_message = message;
    });
    
    // Test progress reporting
    testable.test_report_progress(50, "Testing progress");
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(last_percentage, 50);
    EXPECT_EQ(last_message, "Testing progress");
    
    // Reset and test with no callback
    callback_called = false;
    last_percentage = -1;
    last_message = "";
    
    MockRepoAnalyzer no_callback_analyzer;
    no_callback_analyzer.set_fail(false);
    
    // This should not crash even though no callback is set
    class TestableNoCallbackAnalyzer : public MockRepoAnalyzer {
    public:
        void test_report_progress(int percentage, const std::string& message) {
            report_progress(percentage, message);
        }
    };
    
    TestableNoCallbackAnalyzer testable_no_callback;
    testable_no_callback.test_report_progress(75, "Should not crash");
    
    EXPECT_FALSE(callback_called);
}

TEST(RepoAnalyzerFactoryTest, CreateAnalyzerGithub) {
    // The factory should recognize GitHub URLs and create a GitHubApiAnalyzer
    auto analyzer = singularity::RepoAnalyzerFactory::create_analyzer("https://github.com/user/repo.git");
    EXPECT_NE(analyzer, nullptr);
    EXPECT_NE(dynamic_cast<singularity::GitHubApiAnalyzer*>(analyzer.get()), nullptr);
}

TEST(RepoAnalyzerFactoryTest, CreateAnalyzerUnsupported) {
    // For unsupported repository types, the factory should throw an exception
    EXPECT_THROW(
        singularity::RepoAnalyzerFactory::create_analyzer("https://gitlab.com/user/repo.git"),
        std::runtime_error
    );
}

TEST(RepoAnalyzerFactoryTest, CreateAnalyzerInvalidURL) {
    // For invalid URLs, the factory should throw an exception
    EXPECT_THROW(
        singularity::RepoAnalyzerFactory::create_analyzer("not-a-url"),
        std::runtime_error
    );
}

} // namespace
