#include "singularity/analyzers.hpp"
#include "singularity/language_detector.hpp"
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
#include <set>

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

// Generate a temporary directory name
std::string generate_temp_dir() {
    static std::random_device rd;  
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << fs::temp_directory_path().string() << "/singularity_" << std::hex << dis(gen) << dis(gen);
    return ss.str();
}

}  // namespace

namespace singularity {

// LocalRepoAnalyzer implementation

LocalRepoAnalyzer::LocalRepoAnalyzer(std::string repo_path)
    : repo_path_(std::move(repo_path)) {
    load_language_definitions();
}

LanguageStats LocalRepoAnalyzer::analyze() {
    LanguageStats stats;
    if (!fs::exists(repo_path_)) {
        throw std::runtime_error("Repository path does not exist: " + repo_path_);
    }

    if (!fs::exists(repo_path_ + "/.git")) {
        throw std::runtime_error("Not a git repository: " + repo_path_);
    }

    // Initialize git library
    git_libgit2_init();

    try {
        // Open repository
        git_repository* repo = nullptr;
        int error = git_repository_open(&repo, repo_path_.c_str());
        if (error < 0) {
            const git_error* e = git_error_last();
            throw std::runtime_error(std::string("Unable to open repository: ") + e->message);
        }

        // Get file list from git (to avoid analyzing ignored files)
        std::vector<std::string> files;
        git_index* index = nullptr;
        error = git_repository_index(&index, repo);
        if (error < 0) {
            git_repository_free(repo);
            const git_error* e = git_error_last();
            throw std::runtime_error(std::string("Unable to get repository index: ") + e->message);
        }

        size_t entry_count = git_index_entrycount(index);
        for (size_t i = 0; i < entry_count; ++i) {
            const git_index_entry* entry = git_index_get_byindex(index, i);
            if (entry) {
                files.push_back(entry->path);
            }
        }

        // Process each file
        size_t total_files = files.size();
        for (size_t i = 0; i < total_files; ++i) {
            const auto& file = files[i];
            if (!should_ignore_file(file)) {
                process_file(repo_path_ + "/" + file, stats);
            }
            
            // Report progress every 5% or at least 10 times
            if (i % std::max<size_t>(1, total_files / 20) == 0) {
                int progress = static_cast<int>((i + 1) * 100.0 / total_files);
                std::ostringstream msg;
                msg << "Analyzed " << (i + 1) << " of " << total_files << " files";
                report_progress(progress, msg.str());
            }
        }

        git_index_free(index);
        git_repository_free(repo);
    } catch (...) {
        git_libgit2_shutdown();
        throw;
    }

    git_libgit2_shutdown();
    return stats;
}

void LocalRepoAnalyzer::process_file(const std::string& file_path, LanguageStats& stats) {
    try {
        std::string language = detect_language(file_path);
        if (!language.empty()) {
            size_t size = LanguageDetector::get_file_size(file_path);
            stats.add_file(file_path, language, size);
        }
    } catch (const std::exception& e) {
        // Just skip problematic files
    }
}

std::string LocalRepoAnalyzer::detect_language(const std::string& file_path) {
    return LanguageDetector::instance().detect_language(file_path);
}

bool LocalRepoAnalyzer::should_ignore_file(const std::string& file_path) {
    // Common directories to ignore
    static const std::vector<std::string> ignored_dirs = {
        "node_modules/", "vendor/", ".git/", ".idea/", ".vscode/",
        "build/", "dist/", "target/", "__pycache__/"
    };

    // Check if the file is in an ignored directory
    for (const auto& dir : ignored_dirs) {
        if (file_path.find(dir) != std::string::npos) {
            return true;
        }
    }

    // Check against patterns from gitignore
    return LanguageDetector::path_matches_patterns(file_path, ignored_patterns_);
}

void LocalRepoAnalyzer::load_language_definitions() {
    // Load common file extensions
    extension_to_language_ = {
        {".py", "Python"},
        {".js", "JavaScript"},
        {".jsx", "JavaScript"},
        {".ts", "TypeScript"},
        {".tsx", "TypeScript"},
        {".java", "Java"},
        {".cpp", "C++"},
        {".cc", "C++"},
        {".hpp", "C++"},
        {".h", "C/C++"},
        {".c", "C"},
        {".rb", "Ruby"},
        {".php", "PHP"},
        {".go", "Go"},
        {".rs", "Rust"},
        {".swift", "Swift"},
        {".kt", "Kotlin"},
        {".scala", "Scala"},
        {".cs", "C#"},
        {".fs", "F#"},
        {".sh", "Shell"},
        {".bash", "Shell"},
        {".zsh", "Shell"},
        {".html", "HTML"},
        {".css", "CSS"},
        {".scss", "SCSS"},
        {".sass", "SASS"},
        {".less", "Less"},
        {".xml", "XML"},
        {".json", "JSON"},
        {".yml", "YAML"},
        {".yaml", "YAML"},
        {".md", "Markdown"},
        {".sql", "SQL"},
        {".dart", "Dart"},
        {".lua", "Lua"},
        {".r", "R"},
        {".pl", "Perl"},
        {".pm", "Perl"},
        {".groovy", "Groovy"},
        {".ps1", "PowerShell"},
        {".elm", "Elm"},
        {".clj", "Clojure"},
        {".erl", "Erlang"},
        {".ex", "Elixir"},
        {".exs", "Elixir"},
        {".hs", "Haskell"},
        {".ml", "OCaml"}
    };

    // Load common patterns to ignore
    ignored_patterns_ = {
        "*.min.js", "*.min.css", "*.bundle.js", "*.bundle.css",
        "*.lock", "package-lock.json", "yarn.lock", "Gemfile.lock",
        "*.log", "*.bak", "*.backup", "*.swp", "*~", "*.tmp"
    };
}

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
        report_progress(0, "Fetching language data from GitHub API...");

        // Make API request to get languages
        std::string endpoint = "/repos/" + owner_ + "/" + repo_ + "/languages";
        std::string response = make_api_request(endpoint);

        // Simple parsing of the response to extract language names
        // GitHub API returns languages in format: {"Java":123456,"Python":23456,...}
        std::regex lang_regex("\"([^\"]+)\"");
        std::smatch match;
        std::string::const_iterator search_start(response.cbegin());
        
        std::set<std::string> unique_languages;
        while (std::regex_search(search_start, response.cend(), match, lang_regex)) {
            std::string potential_lang = match[1].str();
            // Skip non-language JSON keys
            if (potential_lang != "url" && potential_lang != "message" && 
                potential_lang != "documentation_url" && !potential_lang.empty()) {
                unique_languages.insert(potential_lang);
            }
            search_start = match.suffix().first;
        }
        
        // Add detected languages with a placeholder size
        for (const auto& lang : unique_languages) {
            stats.add_file("[GitHub API] " + lang, lang, 1);
        }

        report_progress(100, "Completed language analysis");
    } catch (const std::exception& e) {
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

// RemoteRepoAnalyzer implementation

RemoteRepoAnalyzer::RemoteRepoAnalyzer(std::string repo_url)
    : repo_url_(std::move(repo_url)) {
}

RemoteRepoAnalyzer::~RemoteRepoAnalyzer() {
    // Clean up temporary directory if it exists
    if (!temp_dir_.empty() && fs::exists(temp_dir_)) {
        fs::remove_all(temp_dir_);
    }
}

LanguageStats RemoteRepoAnalyzer::analyze() {
    report_progress(0, "Cloning repository...");

    // Clone the repository to a temporary directory
    std::string repo_path = clone_repository();
    
    report_progress(30, "Repository cloned, analyzing...");

    // Create a local analyzer for the cloned repository
    local_analyzer_ = std::make_unique<LocalRepoAnalyzer>(repo_path);
    
    // Forward progress callback
    local_analyzer_->set_progress_callback([this](int percentage, const std::string& message) {
        // Scale progress to the range 30-100%
        int scaled_progress = 30 + (percentage * 70 / 100);
        report_progress(scaled_progress, message);
    });

    // Run the analysis
    return local_analyzer_->analyze();
}

std::string RemoteRepoAnalyzer::clone_repository() {
    // Generate a temporary directory
    temp_dir_ = generate_temp_dir();
    fs::create_directories(temp_dir_);

    // Initialize git
    git_libgit2_init();
    
    try {
        git_repository* repo = nullptr;
        
        // Clone options
        git_clone_options clone_opts;
        git_clone_options_init(&clone_opts, GIT_CLONE_OPTIONS_VERSION);
        clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        
        // Clone the repository
        int error = git_clone(&repo, repo_url_.c_str(), temp_dir_.c_str(), &clone_opts);
        if (error < 0) {
            const git_error* e = git_error_last();
            throw std::runtime_error(std::string("Failed to clone repository: ") + e->message);
        }

        git_repository_free(repo);
        git_libgit2_shutdown();
        
        return temp_dir_;
    } catch (...) {
        git_libgit2_shutdown();
        throw;
    }
}

} // namespace singularity
