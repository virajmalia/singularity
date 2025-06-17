#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace singularity {

/**
 * @brief Class that represents language detection results
 */
class LanguageStats {
public:
    /**
     * @brief Adds file to statistics
     * @param file_path Path to the file
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
     * @brief Get files for a specific language
     * @param language Language name
     * @return Vector of file paths
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
    struct LanguageData {
        size_t total_size{0};
        std::vector<std::string> files;
    };

    std::map<std::string, LanguageData> language_data_;
    size_t total_size_{0};
};

} // namespace singularity
