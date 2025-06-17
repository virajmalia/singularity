#include "singularity/formatter.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace singularity {

std::string TextFormatter::format(const LanguageStats& stats) {
    std::ostringstream out;
    auto languages = stats.get_languages();

    if (languages.empty()) {
        out << "No languages detected in this repository.\n";
        return out.str();
    }

    out << "Languages detected in this repository:\n";
    out << "-------------------------------------\n";

    // Format each language as a simple list
    for (const auto& lang : languages) {
        out << "- " << lang << "\n";
    }

    out << "\nTotal languages detected: " << languages.size() << "\n";
    
    return out.str();
}

} // namespace singularity
