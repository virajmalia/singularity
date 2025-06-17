#pragma once

#include <string>
#include <map>
#include <vector>

namespace singularity {

/**
 * @brief Utility class for language detection
 */
class LanguageDetector {
public:
    /**
     * @brief Get singleton instance
     * @return Singleton instance
     */
    static LanguageDetector& instance();

    /**
     * @brief Initialize language definitions
     */
    void initialize();

    /**
     * @brief Detect language from file extension and content
     * @param file_path Path to the file
     * @param read_content Whether to read content for ambiguous extensions
     * @return Detected language or empty string if unknown
     */
    std::string detect_language(const std::string& file_path, bool read_content = true);

    /**
     * @brief Get file size
     * @param file_path Path to the file
     * @return File size in bytes or 0 if error
     */
    static size_t get_file_size(const std::string& file_path);

    /**
     * @brief Check if a path matches any pattern in the list
     * @param path Path to check
     * @param patterns List of glob patterns
     * @return True if the path matches any pattern
     */
    static bool path_matches_patterns(const std::string& path, const std::vector<std::string>& patterns);

private:
    LanguageDetector();
    ~LanguageDetector() = default;
    
    LanguageDetector(const LanguageDetector&) = delete;
    LanguageDetector& operator=(const LanguageDetector&) = delete;

    struct LanguageDefinition {
        std::vector<std::string> extensions;
        std::vector<std::string> filenames;
        std::map<std::string, std::vector<std::string>> heuristics;
    };

    std::map<std::string, LanguageDefinition> languages_;
    std::map<std::string, std::vector<std::string>> extension_conflicts_;
};

} // namespace singularity
