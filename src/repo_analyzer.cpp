#include "singularity/repo_analyzer.hpp"
#include "singularity/analyzers.hpp"
#include <memory>

namespace singularity {

void RepoAnalyzer::set_progress_callback(std::function<void(int, const std::string&)> callback) {
    progress_callback_ = std::move(callback);
}

void RepoAnalyzer::report_progress(int percentage, const std::string& message) {
    if (progress_callback_) {
        progress_callback_(percentage, message);
    }
}

std::unique_ptr<RepoAnalyzer> RepoAnalyzerFactory::create_local_analyzer(const std::string& path) {
    return std::make_unique<LocalRepoAnalyzer>(path);
}

std::unique_ptr<RepoAnalyzer> RepoAnalyzerFactory::create_remote_analyzer(
    const std::string& url, bool use_api) {
    
    if (use_api && url.find("github.com") != std::string::npos) {
        // Use GitHub API for GitHub repositories if requested
        return std::make_unique<GitHubApiAnalyzer>(url);
    }

    // Use generic remote analyzer for other repositories
    return std::make_unique<RemoteRepoAnalyzer>(url);
}

} // namespace singularity
