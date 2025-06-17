#include "singularity/language_stats.hpp"
#include <algorithm>
#include <numeric>

namespace singularity {

void LanguageStats::add_file(const std::string& file_path, const std::string& language, size_t size) {
    if (language.empty()) {
        return;
    }

    language_data_[language].files.push_back(file_path);
    language_data_[language].total_size += size;
    total_size_ += size;
}

size_t LanguageStats::get_language_size(const std::string& language) const {
    auto it = language_data_.find(language);
    return (it != language_data_.end()) ? it->second.total_size : 0;
}

double LanguageStats::get_language_percentage(const std::string& language) const {
    if (total_size_ == 0) {
        return 0.0;
    }

    auto size = get_language_size(language);
    return static_cast<double>(size) / total_size_ * 100.0;
}

std::vector<std::string> LanguageStats::get_languages() const {
    std::vector<std::string> languages;
    languages.reserve(language_data_.size());

    for (const auto& [lang, _] : language_data_) {
        languages.push_back(lang);
    }

    // Sort by size (descending)
    std::sort(languages.begin(), languages.end(), [this](const std::string& a, const std::string& b) {
        return get_language_size(a) > get_language_size(b);
    });

    return languages;
}

std::vector<std::string> LanguageStats::get_files_for_language(const std::string& language) const {
    auto it = language_data_.find(language);
    return (it != language_data_.end()) ? it->second.files : std::vector<std::string>{};
}

size_t LanguageStats::get_total_size() const {
    return total_size_;
}

void LanguageStats::clear() {
    language_data_.clear();
    total_size_ = 0;
}

} // namespace singularity
