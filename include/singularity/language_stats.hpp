#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace singularity {

/**
 * @brief Class that represents language detection results
 * 
 * Simplified to focus only on languages and their percentages, which are
 * the primary data needed for the application. File paths are no longer tracked
 * individually to reduce memory overhead.
 */
class LanguageStats {
public:
    /**
     * @brief Adds file to statistics
     * @param file_path Path to the file (for compatibility, now ignored)
     * @param language Detected language
     * @param size Size of the file in bytes
     */
    void add_file(const std::string& file_path, const std::string& language, size_t size);

    /**
     * @brief Get total number of bytes for a language
     * @param language Language name
     * @return Total bytes or 0 if language not found
     */
    size_t get_language_size(const std::string& language) const;

    /**
     * @brief Get percentage of a language in the project
     * @param language Language name
     * @return Percentage (0-100) or 0 if language not found
     */
    double get_language_percentage(const std::string& language) const;

    /**
     * @brief Get all detected languages
     * @return Vector of language names
     */
    std::vector<std::string> get_languages() const;

    /**
     * @brief Get files for a specific language (for API compatibility)
     * @param language Language name
     * @return Empty vector, as files are no longer tracked individually
     */
    std::vector<std::string> get_files_for_language(const std::string& language) const;

    /**
     * @brief Get total size of all files
     * @return Total size in bytes
     */
    size_t get_total_size() const;

    /**
     * @brief Clear all statistics
     */
    void clear();

private:
    // Store language data directly without tracking individual files
    std::map<std::string, size_t> language_sizes_;
    size_t total_size_{0};
};

} // namespace singularity
