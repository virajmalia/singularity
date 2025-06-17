#pragma once

#include "language_stats.hpp"
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace singularity {

/**
 * @brief Abstract class for repository analyzers
 */
class RepoAnalyzer {
public:
    virtual ~RepoAnalyzer() = default;

    /**
     * @brief Analyze a repository to detect languages
     * @return Language statistics
     */
    virtual LanguageStats analyze() = 0;

    /**
     * @brief Set a callback for progress updates
     * @param callback Function that takes percentage (0-100) and status message
     */
    void set_progress_callback(std::function<void(int, const std::string&)> callback);

protected:
    /**
     * @brief Report progress to the registered callback
     * @param percentage Progress percentage (0-100)
     * @param message Status message
     */
    void report_progress(int percentage, const std::string& message);

private:
    std::function<void(int, const std::string&)> progress_callback_;
};

/**
 * @brief Factory for creating repository analyzers
 */
class RepoAnalyzerFactory {
public:
    /**
     * @brief Create an analyzer for a local git repository
     * @param path Path to the local repository
     * @return RepoAnalyzer instance
     */
    static std::unique_ptr<RepoAnalyzer> create_local_analyzer(const std::string& path);

    /**
     * @brief Create an analyzer for a remote git repository
     * @param url URL of the remote repository
     * @param use_api Whether to use GitHub API (if applicable)
     * @return RepoAnalyzer instance
     */
    static std::unique_ptr<RepoAnalyzer> create_remote_analyzer(
        const std::string& url, bool use_api = true);
};

} // namespace singularity
