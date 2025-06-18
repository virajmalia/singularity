#pragma once

#include "singularity/language_stats.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace singularity {

/**
 * @brief Class representing a security tool recommendation
 */
struct SecurityTool {
    std::string name;           ///< Name of the tool
    std::string description;    ///< Short description of the tool
    std::string url;            ///< URL to the tool's website or repository
    std::string installation;   ///< Installation instructions
    std::string configuration;  ///< Basic configuration guidance
};

/**
 * @brief Class representing a security scan type
 */
enum class SecurityScanType {
    STATIC_ANALYSIS,
    DEPENDENCY_VULNERABILITY,
    SECRET_DETECTION,
    SAST,
    DAST,
    SECURITY_LINTING,
    CONTAINER_SECURITY,
    CODE_QUALITY,
    LICENSE_COMPLIANCE
};

/**
 * @brief Convert scan type to string
 * @param type Scan type
 * @return String representation of scan type
 */
std::string scan_type_to_string(SecurityScanType type);

/**
 * @brief Class for generating security recommendations based on detected languages
 */
class SecurityRecommender {
public:
    /**
     * @brief Get singleton instance
     * @return Singleton instance
     */
    static SecurityRecommender& instance();

    /**
     * @brief Initialize security recommendations database
     */
    void initialize();

    /**
     * @brief Generate security recommendations based on detected languages
     * @param stats Language statistics from repository analysis
     * @return Generated markdown report
     */
    std::string generate_recommendations(const LanguageStats& stats);

    /**
     * @brief Get recommended tools for a specific language and scan type
     * @param language The programming language
     * @param scan_type Type of security scan
     * @return Vector of recommended security tools
     */
    std::vector<SecurityTool> get_tools_for_language(
        const std::string& language, 
        SecurityScanType scan_type);

    /**
     * @brief Get recommended configuration for a specific language and scan type
     * @param language The programming language
     * @param scan_type Type of security scan
     * @return Configuration string or empty if not found
     */
    std::string get_config_for_language(
        const std::string& language,
        SecurityScanType scan_type);

    /**
     * @brief Get general security tools that apply to any codebase
     * @param scan_type Type of security scan
     * @return Vector of recommended security tools
     */
    std::vector<SecurityTool> get_general_tools(SecurityScanType scan_type);

    /**
     * @brief Generate a GitHub Actions workflow for security scanning
     * @param stats Language statistics from repository analysis
     * @return YAML content for GitHub workflow
     */
    std::string generate_github_workflow(const LanguageStats& stats);

private:
    SecurityRecommender();
    SecurityRecommender(const SecurityRecommender&) = delete;
    SecurityRecommender& operator=(const SecurityRecommender&) = delete;

    /**
     * @brief Add tool recommendations for a specific language and scan type
     * @param language The programming language
     * @param scan_type Type of security scan
     * @param tools Vector of security tools
     * @param config Configuration guidance
     */
    void add_language_tools(
        const std::string& language,
        SecurityScanType scan_type,
        std::vector<SecurityTool> tools,
        const std::string& config);

    /**
     * @brief Add general tool recommendations for a specific scan type
     * @param scan_type Type of security scan
     * @param tools Vector of security tools
     * @param config Configuration guidance
     */
    void add_general_tools(
        SecurityScanType scan_type,
        std::vector<SecurityTool> tools,
        const std::string& config);

    /**
     * @brief Generate a section of the report for a specific scan type
     * @param type Scan type
     * @param stats Language statistics
     * @return Generated markdown section
     */
    std::string generate_section(SecurityScanType type, const LanguageStats& stats);

    /**
     * @brief Generate a language-specific subsection
     * @param language The programming language
     * @param scan_type Type of security scan
     * @return Generated markdown subsection
     */
    std::string generate_language_subsection(
        const std::string& language,
        SecurityScanType scan_type);

    /**
     * @brief Generate GitHub workflow steps for a specific language
     * @param language The programming language
     * @return YAML content for workflow steps
     */
    std::string generate_workflow_steps(const std::string& language);

    // Data structures to store recommendations
    struct ScanTypeRecommendations {
        std::map<std::string, std::vector<SecurityTool>> language_tools;
        std::map<std::string, std::string> language_configs;
        std::vector<SecurityTool> general_tools;
        std::string general_config;
    };

    std::map<SecurityScanType, ScanTypeRecommendations> recommendations_;
    bool initialized_ = false;
};

} // namespace singularity
