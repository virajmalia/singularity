#pragma once

#include "language_stats.hpp"
#include "repo_analyzer.hpp"
#include <string>
#include <memory>

namespace singularity {

/**
 * @brief Analyzer that uses GitHub API to get language statistics for GitHub repositories
 *
 * This analyzer directly calls the GitHub API to get language statistics for a given
 * GitHub repository URL. It implements the RepoAnalyzer interface to support GitHub repositories.
 *
 * This is currently the only implemented analyzer, but the design allows for adding
 * support for other repository hosts (like GitLab) in the future.
 */
class GitHubApiAnalyzer : public RepoAnalyzer {
public:
    /**
     * @brief Constructor
     * @param repo_url URL of the GitHub repository
     * @throws std::runtime_error if the URL is not a valid GitHub repository URL
     */
    explicit GitHubApiAnalyzer(std::string repo_url);

    /**
     * @brief Analyze the repository using GitHub API
     * @return Language statistics
     */
    LanguageStats analyze() override;

private:
    /**
     * @brief Extract owner and repo name from GitHub URL
     * @param url GitHub repository URL
     * @return Pair of owner and repo name, or empty strings if invalid
     */
    std::pair<std::string, std::string> extract_repo_info(const std::string& url);

    /**
     * @brief Make a GitHub API request
     * @param endpoint API endpoint
     * @return API response as string
     */
    std::string make_api_request(const std::string& endpoint);

    std::string repo_url_;
    std::string owner_;
    std::string repo_;
};

} // namespace singularity
