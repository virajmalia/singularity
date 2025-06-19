#include <git2.h>
#include <curl/curl.h>
#include <filesystem>
#include <random>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <set>

#include "singularity/analyzers.hpp"

namespace fs = std::filesystem;

namespace {

// Callback for CURL to write received data to a string
size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t new_length = size * nmemb;
    try {
        s->append(static_cast<char*>(contents), new_length);
        return new_length;
    } catch (const std::bad_alloc& e) {
        return 0;
    }
}

}  // namespace

namespace singularity {

// GitHubApiAnalyzer implementation

GitHubApiAnalyzer::GitHubApiAnalyzer(std::string repo_url)
    : repo_url_(std::move(repo_url)) {

    // Extract owner and repo from URL
    std::tie(owner_, repo_) = extract_repo_info(repo_url_);
    if (owner_.empty() || repo_.empty()) {
        throw std::runtime_error("Invalid GitHub URL: " + repo_url_);
    }
}

LanguageStats GitHubApiAnalyzer::analyze() {
    LanguageStats stats;

    try {
        // Report start
        std::cout << "Analyzing GitHub repository: " << owner_ << "/" << repo_ << std::endl;
        report_progress(0, "Fetching language data from GitHub API...");

        // Make API request to get languages
        std::string endpoint = "/repos/" + owner_ + "/" + repo_ + "/languages";
        std::string response = make_api_request(endpoint);

        stats.set_repo_url(repo_url_);

        // Check if response contains an error message
        if (response.find("\"message\":") != std::string::npos &&
            (response.find("\"Not Found\"") != std::string::npos ||
             response.find("\"Bad credentials\"") != std::string::npos)) {
            throw std::runtime_error("GitHub API error: Repository not found or access denied");
        }

        // More accurate parsing of the response to extract language names and byte counts
        // GitHub API returns languages in format: {"Java":123456,"Python":23456,...}
        std::regex lang_regex("\"([^\"]+)\":\\s*(\\d+)");
        std::smatch match;
        std::string::const_iterator search_start(response.cbegin());

        // First, gather all language sizes
        std::map<std::string, size_t> language_sizes;
        while (std::regex_search(search_start, response.cend(), match, lang_regex)) {
            std::string language = match[1].str();
            size_t size = std::stoull(match[2].str());

            // Skip non-language JSON keys
            if (language != "url" && language != "message" &&
                language != "documentation_url" && !language.empty()) {
                language_sizes[language] = size;
            }
            search_start = match.suffix().first;
        }

        // Check if we found any languages
        if (language_sizes.empty()) {
            std::cout << "Warning: No languages detected via GitHub API" << std::endl;
            std::cout << "API Response: " << response << std::endl;
        }

        // Add detected languages with their actual byte counts
        for (const auto& [language, size] : language_sizes) {
            //std::cout << "Detected language: " << language << " (" << size << " bytes)" << std::endl;
            stats.add_file(language, size);
        }

        report_progress(100, "Completed language analysis");
    } catch (const std::exception& e) {
        std::cout << "GitHub API error: " << e.what() << std::endl;
        throw std::runtime_error(std::string("GitHub API error: ") + e.what());
    }

    return stats;
}

std::pair<std::string, std::string> GitHubApiAnalyzer::extract_repo_info(const std::string& url) {
    // Match GitHub URL patterns:
    // https://github.com/owner/repo
    // git@github.com:owner/repo.git
    std::regex github_https_regex(R"(github\.com/([^/]+)/([^/\.]+))");
    std::regex github_ssh_regex(R"(github\.com:([^/]+)/([^/\.]+))");

    std::smatch match;
    if (std::regex_search(url, match, github_https_regex) ||
        std::regex_search(url, match, github_ssh_regex)) {
        if (match.size() >= 3) {
            return {match[1].str(), match[2].str()};
        }
    }
    return {"", ""};
}

std::string GitHubApiAnalyzer::make_api_request(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string url = "https://api.github.com" + endpoint;
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Singularity/0.1");

    // Add headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
    }

    // Check response code
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::runtime_error("HTTP error: " + std::to_string(http_code));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

} // namespace singularity
