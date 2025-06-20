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
 *
 * The factory provides methods to create analyzers for different
 * repository hosting services. Currently, only GitHub is implemented,
 * but the design allows for extending to support other hosting services.
 */
class RepoAnalyzerFactory {
public:
    /**
     * @brief Create an appropriate analyzer for a remote repository
     * @param url URL of the remote repository
     * @return RepoAnalyzer instance
     * @throws std::runtime_error if URL format is not supported
     *
     * This factory method examines the URL and creates the appropriate
     * analyzer based on the repository hosting service.
     *
     * Currently supported:
     * - GitHub repositories
     */
    static std::unique_ptr<RepoAnalyzer> create_analyzer(
        const std::string& url);
};

} // namespace singularity
