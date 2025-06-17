#pragma once

#include "language_stats.hpp"
#include "repo_analyzer.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace singularity {

/**
 * @brief Analyzer that detects languages based on file extensions and patterns
 */
class LocalRepoAnalyzer : public RepoAnalyzer {
public:
    /**
     * @brief Constructor
     * @param repo_path Path to the local git repository
     */
    explicit LocalRepoAnalyzer(std::string repo_path);

    /**
     * @brief Analyze the repository
     * @return Language statistics
     */
    LanguageStats analyze() override;

private:
    /**
     * @brief Process a single file
     * @param file_path Path to the file
     * @param stats Statistics to update
     */
    void process_file(const std::string& file_path, LanguageStats& stats);

    /**
     * @brief Detect language based on file extension and content
     * @param file_path Path to the file
     * @return Detected language or empty string if unknown
     */
    std::string detect_language(const std::string& file_path);

    /**
     * @brief Check if a file should be ignored
     * @param file_path Path to the file
     * @return True if the file should be ignored
     */
    bool should_ignore_file(const std::string& file_path);

    /**
     * @brief Load language definitions from configuration
     */
    void load_language_definitions();

    std::string repo_path_;
    std::map<std::string, std::string> extension_to_language_;
    std::vector<std::string> ignored_patterns_;
};

/**
 * @brief Analyzer that uses GitHub API to get language statistics
 */
class GitHubApiAnalyzer : public RepoAnalyzer {
public:
    /**
     * @brief Constructor
     * @param repo_url URL of the GitHub repository
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

/**
 * @brief Analyzer that clones a remote repository and analyzes it locally
 */
class RemoteRepoAnalyzer : public RepoAnalyzer {
public:
    /**
     * @brief Constructor
     * @param repo_url URL of the remote git repository
     */
    explicit RemoteRepoAnalyzer(std::string repo_url);

    /**
     * @brief Destructor - cleans up temporary files
     */
    ~RemoteRepoAnalyzer() override;

    /**
     * @brief Analyze the repository
     * @return Language statistics
     */
    LanguageStats analyze() override;

private:
    /**
     * @brief Clone the repository to a temporary directory
     * @return Path to the cloned repository
     */
    std::string clone_repository();

    std::string repo_url_;
    std::string temp_dir_;
    std::unique_ptr<LocalRepoAnalyzer> local_analyzer_;
};

} // namespace singularity
