#pragma once

#include "singularity/language_stats.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <optional>

namespace singularity {

/**
 * @brief Class representing a MegaLinter workflow generator
 */
class WorkflowGenerator {
public:
    /**
     * @brief Get singleton instance
     * @return Singleton instance
     */
    static WorkflowGenerator& instance();

    /**
     * @brief Initialize the workflow generator
     * @param security_mappings Path to security mappings JSON file
     * @return True if initialization successful
     */
    bool initialize(const std::string& security_mappings = "security_mappings.json");

    /**
     * @brief Generate MegaLinter workflow templates based on detected languages
     * @param stats Language statistics from repository analysis
     * @param output_dir Directory where workflow files should be written
     * @return True if generation successful
     */
    bool generate_workflows(const LanguageStats& stats, const std::string& output_dir = "output");

    /**
     * @brief Check if the workflow generator is initialized
     * @return True if initialized
     */
    bool is_initialized() const { return initialized_; }

private:
    WorkflowGenerator();
    WorkflowGenerator(const WorkflowGenerator&) = delete;
    WorkflowGenerator& operator=(const WorkflowGenerator&) = delete;

    /**
     * @brief Generate a GitHub Actions workflow file for MegaLinter
     * @param scan_type The type of scan (e.g., "static_analysis")
     * @param languages Vector of languages to include in the workflow
     * @param output_dir Directory where workflow file should be written
     * @return True if generation successful
     */
    bool generate_megalinter_workflow(
        const std::string& scan_type,
        const std::vector<std::string>& languages,
        const std::string& output_dir);

    /**
     * @brief Generate MegaLinter configuration file
     * @param languages Vector of languages to include in configuration
     * @param output_dir Directory where configuration file should be written
     * @return True if generation successful
     */
    bool generate_megalinter_config(
        const std::vector<std::string>& languages,
        const std::string& output_dir);

    /**
     * @brief Generate a GitHub Actions workflow file for Lizard complexity analysis
     * @param languages Vector of languages to analyze
     * @param output_dir Directory where workflow file should be written
     * @return True if generation successful
     */
    bool generate_lizard_workflow(
        const std::vector<std::string>& languages,
        const std::string& output_dir);

    /**
     * @brief Map Singularity scan types to MegaLinter linter groups
     * @param scan_type The scan type from security_mappings.json
     * @return Corresponding MegaLinter linter group
     */
    std::string map_to_megalinter_group(const std::string& scan_type);

    /**
     * @brief Get appropriate MegaLinter linters for a language and scan type
     * @param language The language name
     * @param scan_type The scan type
     * @return Vector of linter names
     */
    std::vector<std::string> get_linters_for_language(
        const std::string& language,
        const std::string& scan_type);

    /**
     * @brief Get generic MegaLinter linters for a scan type (regardless of language)
     * @param scan_type The scan type
     * @return Vector of linter names
     */
    std::vector<std::string> get_generic_linters_for_scan_type(
        const std::string& scan_type);

    /**
     * @brief Add tool-specific configurations to the workflow file
     * @param workflow_file Output stream for the workflow file
     * @param language The language name
     * @param scan_type The scan type
     */
    void add_tool_configs(
        std::ofstream& workflow_file,
        const std::string& language,
        const std::string& scan_type);

    nlohmann::json security_mappings_;
    bool initialized_ = false;

    // Map tool names to MegaLinter linters
    const std::unordered_map<std::string, std::string> tool_to_linter = {
        {"Cppcheck", "CPP_CPPCHECK"},
        {"CppLint", "CPP_CPPLINT"},
        {"clang-tidy", "CPP_CLANG_TIDY"},
        {"Clang Static Analyzer", "CPP_CLANG_TIDY"},
        {"FlawFinder", "CPP_FLAWFINDER"},
        {"Bandit", "PYTHON_BANDIT"},
        {"Pylint", "PYTHON_PYLINT"},
        {"Black", "PYTHON_BLACK"},
        {"Safety", "PYTHON_SAFETY"},
        {"Semgrep", "REPOSITORY_SEMGREP"},
        {"GitLeaks", "REPOSITORY_GITLEAKS"},
        {"Secretlint", "REPOSITORY_SECRETLINT"},
        {"OWASP Dependency-Check", "REPOSITORY_DEPENDENCY_CHECK"},
        {"Dependabot", "REPOSITORY_DEPENDABOT"},
        {"Fortify", "REPOSITORY_FORTIFY"},
        {"Detect-secrets", "REPOSITORY_SECRETLINT"},
        {"OWASP ZAP", "REPOSITORY_ZAP"},
        {"licensechecker", "REPOSITORY_LICENSECHECKER"},
        {"scancode-toolkit", "REPOSITORY_SCANCODE"},
        {"REUSE Tool", "REPOSITORY_REUSE"},
        {"Trivy", "REPOSITORY_TRIVY"}
    };
};

} // namespace singularity
