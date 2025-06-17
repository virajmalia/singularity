#pragma once

#include "language_stats.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace singularity {

/**
 * @brief Formatter for language list output
 */
class Formatter {
public:
    virtual ~Formatter() = default;

    /**
     * @brief Format language statistics 
     * @param stats Language statistics to format
     * @return Formatted string
     */
    virtual std::string format(const LanguageStats& stats) = 0;
};

/**
 * @brief Text formatter for console output
 * Provides a simple list of languages detected
 */
class TextFormatter : public Formatter {
public:
    /**
     * @brief Format language list as text
     * @param stats Language statistics to format
     * @return Formatted text list of languages
     */
    std::string format(const LanguageStats& stats) override;
};

} // namespace singularity
