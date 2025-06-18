#include "singularity/repo_analyzer.hpp"
#include "singularity/analyzers.hpp"
#include <memory>
#include <iostream>

namespace singularity {

void RepoAnalyzer::set_progress_callback(std::function<void(int, const std::string&)> callback) {
    progress_callback_ = std::move(callback);
}

void RepoAnalyzer::report_progress(int percentage, const std::string& message) {
    if (progress_callback_) {
        progress_callback_(percentage, message);
    }
}

std::unique_ptr<RepoAnalyzer> RepoAnalyzerFactory::create_analyzer(
    const std::string& url) {

    // Determine repository type from URL
    if (url.find("github.com") != std::string::npos) {
        std::cout << "Detected GitHub repository, using GitHub API for analysis" << std::endl;
        return std::make_unique<GitHubApiAnalyzer>(url);
    }

    // This is where we would add support for other repository types
    // For example:
    // if (url.find("gitlab.com") != std::string::npos) {
    //     return std::make_unique<GitLabApiAnalyzer>(url);
    // }

    // If we don't recognize the repository type
    throw std::runtime_error("Unsupported repository URL. Currently only GitHub repositories are supported.");
}

} // namespace singularity
