#include "singularity/language_stats.hpp"
#include <algorithm>
#include <numeric>

namespace singularity {

void LanguageStats::add_file(const std::string& language, size_t size) {
    if (language.empty()) {
        return;
    }

    language_sizes_[language] += size;
    total_size_ += size;
}

size_t LanguageStats::get_language_size(const std::string& language) const {
    auto it = language_sizes_.find(language);
    return (it != language_sizes_.end()) ? it->second : 0;
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
    languages.reserve(language_sizes_.size());

    for (const auto& [lang, _] : language_sizes_) {
        languages.push_back(lang);
    }

    // Sort by size (descending)
    std::sort(languages.begin(), languages.end(), [this](const std::string& a, const std::string& b) {
        return get_language_size(a) > get_language_size(b);
    });

    return languages;
}

size_t LanguageStats::get_total_size() const {
    return total_size_;
}

void LanguageStats::clear() {
    language_sizes_.clear();
    total_size_ = 0;
}

} // namespace singularity
