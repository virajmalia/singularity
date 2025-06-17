#include "singularity/language_detector.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

namespace {

// Helper function to read the first N bytes of a file
std::string read_file_head(const std::string& file_path, size_t bytes = 4096) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return {};
    }

    std::string content;
    content.resize(bytes);
    file.read(&content[0], bytes);
    content.resize(file.gcount());
    
    return content;
}

// Helper function to get file extension
std::string get_file_extension(const std::string& file_path) {
    fs::path path(file_path);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return ext;
}

// Helper function to check if content matches a pattern
bool content_matches_pattern(const std::string& content, const std::string& pattern) {
    try {
        std::regex regex(pattern);
        return std::regex_search(content, regex);
    } catch (const std::regex_error&) {
        // Invalid regex pattern
        return false;
    }
}

// Convert glob pattern to regex pattern
std::string glob_to_regex(const std::string& glob) {
    std::string regex;
    regex.reserve(glob.size() + 10);
    
    bool in_bracket = false;
    
    for (char c : glob) {
        switch (c) {
            case '*':
                regex += in_bracket ? "*" : ".*";
                break;
            case '?':
                regex += in_bracket ? "?" : ".";
                break;
            case '.':
                regex += "\\.";
                break;
            case '\\':
                regex += "\\\\";
                break;
            case '+':
                regex += "\\+";
                break;
            case '(':
                regex += "\\(";
                break;
            case ')':
                regex += "\\)";
                break;
            case '^':
                regex += "\\^";
                break;
            case '$':
                regex += "\\$";
                break;
            case '[':
                in_bracket = true;
                regex += c;
                break;
            case ']':
                in_bracket = false;
                regex += c;
                break;
            default:
                regex += c;
                break;
        }
    }
    
    return "^" + regex + "$";
}

} // namespace

namespace singularity {

LanguageDetector& LanguageDetector::instance() {
    static LanguageDetector instance;
    return instance;
}

LanguageDetector::LanguageDetector() {
    initialize();
}

void LanguageDetector::initialize() {
    // Initialize language definitions - this is a simplified version
    // In a real implementation, this would likely be loaded from a database/file
    
    // Initialize common languages
    languages_["C++"] = {
        {".cpp", ".cc", ".cxx", ".c++", ".hpp", ".hh", ".hxx", ".h++"},
        {"Makefile.am"},
        {
            {".h", {"\\b(class|namespace|template|typedef|using)\\b"}}
        }
    };
    
    languages_["C"] = {
        {".c"},
        {},
        {
            {".h", {"\\b(typedef|struct|enum|union)\\b(?!.*\\bclass\\b)"}}
        }
    };
    
    languages_["Python"] = {
        {".py", ".pyw", ".pyc", ".pyo", ".pyd"},
        {"requirements.txt", "setup.py", "Pipfile", "pyproject.toml"},
        {}
    };
    
    languages_["JavaScript"] = {
        {".js", ".mjs", ".cjs"},
        {"package.json", ".eslintrc", ".babelrc"},
        {}
    };
    
    languages_["TypeScript"] = {
        {".ts", ".tsx"},
        {"tsconfig.json", "tslint.json"},
        {}
    };
    
    languages_["Java"] = {
        {".java", ".class", ".jar"},
        {"pom.xml", "build.gradle", ".java-version"},
        {}
    };
    
    languages_["Ruby"] = {
        {".rb", ".rake", ".gemspec"},
        {"Gemfile", "Rakefile", ".ruby-version"},
        {}
    };
    
    languages_["Go"] = {
        {".go"},
        {"go.mod", "go.sum", ".go-version"},
        {}
    };
    
    languages_["Rust"] = {
        {".rs"},
        {"Cargo.toml", "Cargo.lock", "rust-toolchain"},
        {}
    };
    
    languages_["PHP"] = {
        {".php", ".phtml", ".php5", ".php7"},
        {"composer.json", ".php-version"},
        {}
    };
    
    languages_["Swift"] = {
        {".swift"},
        {"Package.swift"},
        {}
    };
    
    languages_["Kotlin"] = {
        {".kt", ".kts"},
        {},
        {}
    };
    
    languages_["HTML"] = {
        {".html", ".htm", ".xhtml"},
        {},
        {}
    };
    
    languages_["CSS"] = {
        {".css"},
        {},
        {}
    };
    
    languages_["Shell"] = {
        {".sh", ".bash", ".zsh", ".ksh"},
        {".bashrc", ".bash_profile", ".zshrc"},
        {}
    };

    languages_["CMake"] = {
        {".cmake", ".cmake.in"},
        {"CMakeLists.txt"},
        {}
    };
    
    // Build extension conflicts map
    extension_conflicts_[".h"] = {"C++", "C", "Objective-C"};
    extension_conflicts_[".m"] = {"Objective-C", "MATLAB"};
}

std::string LanguageDetector::detect_language(const std::string& file_path, bool read_content) {
    // Get file extension
    std::string extension = get_file_extension(file_path);
    
    // Get filename
    fs::path path(file_path);
    std::string filename = path.filename().string();
    
    // First check filename-based matches (highest precedence)
    for (const auto& [lang, def] : languages_) {
        // Check by exact filename
        for (const auto& name : def.filenames) {
            if (name == filename) {
                return lang;
            }
        }
    }
    
    // Special case for .h files - default to C++ unless shown otherwise
    if (extension == ".h") {
        if (read_content) {
            std::string content = read_file_head(file_path);
            // Check for C-specific patterns
            for (const auto& pattern : languages_["C"].heuristics.at(".h")) {
                if (content_matches_pattern(content, pattern)) {
                    return "C";
                }
            }
        }
        return "C++";  // Default to C++ for .h files
    }
    
    // Check extension matches
    for (const auto& [lang, def] : languages_) {
        // Check by extension
        for (const auto& ext : def.extensions) {
            if (ext == extension) {
                // If it's a conflicting extension and we're allowed to read content
                auto conflict_it = extension_conflicts_.find(extension);
                if (read_content && conflict_it != extension_conflicts_.end() && 
                    std::find(conflict_it->second.begin(), conflict_it->second.end(), lang) != conflict_it->second.end()) {
                    // Need to check content to disambiguate
                    const auto& heuristics = def.heuristics;
                    auto heur_it = heuristics.find(extension);
                    if (heur_it != heuristics.end()) {
                        // We have heuristics for this extension, check them
                        std::string content = read_file_head(file_path);
                        for (const auto& pattern : heur_it->second) {
                            if (content_matches_pattern(content, pattern)) {
                                return lang;
                            }
                        }
                    } else {
                        // No heuristics, treat as a match anyway
                        return lang;
                    }
                } else {
                    // Not conflicting, just return the language
                    return lang;
                }
            }
        }
        
        // Check by exact filename
        for (const auto& name : def.filenames) {
            if (name == filename) {
                return lang;
            }
        }
    }
    
    // No match found
    return "";
}

size_t LanguageDetector::get_file_size(const std::string& file_path) {
    try {
        return fs::file_size(file_path);
    } catch (const fs::filesystem_error&) {
        return 0;
    }
}

bool LanguageDetector::path_matches_patterns(const std::string& path, const std::vector<std::string>& patterns) {
    for (const auto& pattern : patterns) {
        std::string regex_pattern = glob_to_regex(pattern);
        try {
            std::regex regex(regex_pattern);
            if (std::regex_match(path, regex)) {
                return true;
            }
        } catch (const std::regex_error&) {
            // Skip invalid patterns
            continue;
        }
    }
    return false;
}

} // namespace singularity
