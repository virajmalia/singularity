/*
 * Copyright 2025 Singularity Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <nlohmann/json.hpp>

#include "singularity/workflow_generator.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace singularity {

// Singleton implementation
WorkflowGenerator& WorkflowGenerator::instance() {
    static WorkflowGenerator instance;
    return instance;
}

WorkflowGenerator::WorkflowGenerator() {
    // Private constructor for singleton
}

bool WorkflowGenerator::initialize(const std::string& security_mappings) {
    if (initialized_) {
        return true;
    }

    try {
        // Try to open security mappings file
        std::ifstream mappings_file;
        std::vector<std::string> possible_paths = {
            security_mappings,
            "../" + security_mappings,
            "../../" + security_mappings,
            "../../../" + security_mappings
        };

        for (const auto& path : possible_paths) {
            mappings_file.open(path);
            if (mappings_file.is_open()) {
                break;
            }
        }

        if (!mappings_file.is_open()) {
            throw std::runtime_error("Could not open " + security_mappings + " from any expected location");
        }

        // Parse JSON mappings
        security_mappings_ = json::parse(mappings_file);

        initialized_ = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing workflow generator: " << e.what() << std::endl;
        return false;
    }
}

bool WorkflowGenerator::generate_workflows(const LanguageStats& stats, const std::string& output_dir) {
    if (!initialized_) {
        std::cerr << "WorkflowGenerator not initialized! Call initialize() first." << std::endl;
        return false;
    }

    try {
        // Create output directory if it doesn't exist
        fs::create_directories(output_dir);
        fs::create_directories(output_dir + "/workflows");
        fs::create_directories(output_dir + "/config");

        // Get all detected languages
        std::vector<std::string> languages = stats.get_languages();

        // Process each scan type from security_mappings.json
        const json& scan_types = security_mappings_["scan_types"];

        for (auto it = scan_types.begin(); it != scan_types.end(); ++it) {
            const std::string& scan_type = it.key();

            // Generate workflow file for this scan type
            if (!generate_megalinter_workflow(scan_type, languages, output_dir)) {
                std::cerr << "Failed to generate workflow for scan type: " << scan_type << std::endl;
                continue;
            }
        }

        // Generate MegaLinter config file
        if (!generate_megalinter_config(languages, output_dir)) {
            std::cerr << "Failed to generate MegaLinter configuration" << std::endl;
            return false;
        }

        std::cout << "Successfully generated workflow templates in " << output_dir << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error generating workflows: " << e.what() << std::endl;
        return false;
    }
}

bool WorkflowGenerator::generate_megalinter_workflow(
    const std::string& scan_type,
    const std::vector<std::string>& languages,
    const std::string& output_dir) {

    // Create the GitHub Actions workflow file
    std::string workflow_filename = output_dir + "/workflows/megalinter-" + scan_type + ".yml";
    std::ofstream workflow_file(workflow_filename);

    if (!workflow_file.is_open()) {
        std::cerr << "Could not open file for writing: " << workflow_filename << std::endl;
        return false;
    }

    // Map scan type to MegaLinter group
    std::string megalinter_group = map_to_megalinter_group(scan_type);

    // Create workflow content
    workflow_file << "---\n";
    workflow_file << "# MegaLinter GitHub Actions workflow for " << scan_type << " scans\n";
    workflow_file << "name: MegaLinter " << scan_type << "\n\n";
    workflow_file << "on:\n";
    workflow_file << "  push:\n";
    workflow_file << "    branches: [main, master]\n";
    workflow_file << "  pull_request:\n";
    workflow_file << "    branches: [main, master]\n\n";
    workflow_file << "jobs:\n";
    workflow_file << "  megalinter-" << scan_type << ":\n";
    workflow_file << "    name: MegaLinter " << scan_type << " Scan\n";
    workflow_file << "    runs-on: ubuntu-latest\n";
    workflow_file << "    steps:\n";
    workflow_file << "      # Checkout code\n";
    workflow_file << "      - name: Checkout Code\n";
    workflow_file << "        uses: actions/checkout@v3\n";
    workflow_file << "        with:\n";
    workflow_file << "          fetch-depth: 0\n\n";
    workflow_file << "      # MegaLinter\n";
    workflow_file << "      - name: MegaLinter " << scan_type << " Scan\n";
    workflow_file << "        id: ml\n";
    workflow_file << "        uses: oxsecurity/megalinter@v8\n";
    workflow_file << "        env:\n";
    workflow_file << "          VALIDATE_ALL_CODEBASE: true\n";
    workflow_file << "          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}\n";
    workflow_file << "          MEGALINTER_CONFIG: .github/config/.mega-linter.yml\n";

    // Add comment about dynamic language detection
    workflow_file << "          # Dynamically enable linters based on detected languages: ";
    for (const auto& language : languages) {
        workflow_file << language << ", ";
    }
    workflow_file << "\n";

    // Start building the ENABLE_LINTERS list
    workflow_file << "          ENABLE_LINTERS: >-\n";

    // Get specific linters for each detected language and this scan type
    std::set<std::string> enabled_linters; // Use set to avoid duplicates

    for (const auto& language : languages) {
        auto language_linters = get_linters_for_language(language, scan_type);
        for (const auto& linter : language_linters) {
            enabled_linters.insert(linter);
        }
    }

    // Write each linter on a separate line
    for (const auto& linter : enabled_linters) {
        workflow_file << "            " << linter << ",\n";
    }

    // Add generic linters for this scan type regardless of language
    auto generic_linters = get_generic_linters_for_scan_type(scan_type);
    for (const auto& linter : generic_linters) {
        workflow_file << "            " << linter << ",\n";
    }

    // Add tool-specific configurations
    workflow_file << "\n          # Tool-specific configurations\n";
    for (const auto& language : languages) {
        add_tool_configs(workflow_file, language, scan_type);
    }

    // Continue with common configuration
    workflow_file << "\n";
    workflow_file << "      # Upload MegaLinter artifacts\n";
    workflow_file << "      - name: Archive production artifacts\n";
    workflow_file << "        uses: actions/upload-artifact@v3\n";
    workflow_file << "        if: success() || failure()\n";
    workflow_file << "        with:\n";
    workflow_file << "          name: MegaLinter reports\n";
    workflow_file << "          path: |\n";
    workflow_file << "            megalinter-reports\n";
    workflow_file << "            mega-linter.log\n\n";

    workflow_file << "      # Create pull request with fixes if applicable\n";
    workflow_file << "      - name: Create Pull Request with applied fixes\n";
    workflow_file << "        uses: peter-evans/create-pull-request@v4\n";
    workflow_file << "        if: steps.ml.outputs.has_updated_sources == 1 && (env.APPLY_FIXES == 'all' || env.APPLY_FIXES == 'true')\n";
    workflow_file << "        with:\n";
    workflow_file << "          token: ${{ secrets.GITHUB_TOKEN }}\n";
    workflow_file << "          commit-message: \"[MegaLinter] Apply " << scan_type << " fixes\"\n";
    workflow_file << "          title: \"[MegaLinter] Apply " << scan_type << " fixes\"\n";
    workflow_file << "          labels: bot\n";

    workflow_file.close();
    std::cout << "Generated workflow file: " << workflow_filename << std::endl;
    return true;
}

bool WorkflowGenerator::generate_megalinter_config(
    const std::vector<std::string>& languages,
    const std::string& output_dir) {

    // Create directory for config if it doesn't exist
    fs::create_directories(output_dir + "/config");

    // Create the MegaLinter config file
    std::string config_filename = output_dir + "/config/.mega-linter.yml";
    std::ofstream config_file(config_filename);

    if (!config_file.is_open()) {
        std::cerr << "Could not open file for writing: " << config_filename << std::endl;
        return false;
    }

    // Create config content
    config_file << "---\n";
    config_file << "# MegaLinter Configuration file\n";
    config_file << "# Generated by Singularity\n\n";

    config_file << "# Define global configuration\n";
    config_file << "APPLY_FIXES: all\n";
    config_file << "SHOW_ELAPSED_TIME: true\n";
    config_file << "FILEIO_REPORTER: false\n\n";

    // Configure language-specific settings based on detected languages
    config_file << "# Language specific configurations\n";

    for (const auto& language : languages) {
        std::string ml_language = map_language_to_megalinter(language);
        if (!ml_language.empty()) {
            config_file << "# " << language << " configuration\n";

            // Add language-specific linters and their configurations
            for (const auto& scan_type_entry : security_mappings_["scan_types"].items()) {
                const std::string& scan_type = scan_type_entry.key();
                auto linters = get_linters_for_language(language, scan_type);

                // for (const auto& linter : linters) {
                //     config_file << linter << "_ARGUMENTS: \"\"\n";
                //     config_file << linter << "_FILTER_REGEX_INCLUDE: \"\"\n";
                //     config_file << linter << "_FILE_EXTENSIONS: \"\"\n\n";
                // }
            }
        }
    }

    // Add configuration for general tools
    config_file << "# General security scanning configuration\n";
    config_file << "REPOSITORY_GITLEAKS_CONFIG_FILE: .github/config/gitleaks.toml\n";
    config_file << "REPOSITORY_TRIVY_ARGUMENTS: --severity HIGH,CRITICAL\n\n";

    config_file << "# Disable specific linters\n";
    config_file << "DISABLE:\n";
    config_file << "  - COPYPASTE # Optional: disable copy-paste detection\n";
    config_file << "  - SPELL # Optional: disable spell checking\n\n";

    config_file << "# Example custom configuration for linters\n";
    config_file << "PYTHON_BANDIT_ARGUMENTS: -ll -ii\n";

    config_file.close();
    std::cout << "Generated MegaLinter config file: " << config_filename << std::endl;

    return true;
}

std::string WorkflowGenerator::map_to_megalinter_group(const std::string& scan_type) {
    // Map scan types to MegaLinter linter groups
    static const std::unordered_map<std::string, std::string> scan_to_group = {
        {"static_analysis", "SECURITY"},
        {"security_linting", "SECURITY"},
        {"sast", "SECURITY"},
        {"code_quality", "CODE_QUALITY"},
        {"cve_scanning", "REPOSITORY"},
        {"license_compliance", "LICENSE"}
    };

    auto it = scan_to_group.find(scan_type);
    if (it != scan_to_group.end()) {
        return it->second;
    }

    // Default to empty if no mapping found
    return "";
}

std::string WorkflowGenerator::map_language_to_megalinter(const std::string& language) {
    // Map language names to MegaLinter language identifiers
    static const std::unordered_map<std::string, std::string> lang_map = {
        {"C", "C"},
        {"C++", "CPP"},
        {"Python", "PYTHON"},
        {"JavaScript", "JAVASCRIPT"},
        {"TypeScript", "TYPESCRIPT"},
        {"Java", "JAVA"},
        {"Go", "GO"},
        {"Ruby", "RUBY"},
        {"PHP", "PHP"},
        {"C#", "CSHARP"},
        {"Shell", "BASH"},
        {"PowerShell", "POWERSHELL"},
        {"R", "R"},
        {"Kotlin", "KOTLIN"},
        {"Swift", "SWIFT"}
    };

    auto it = lang_map.find(language);
    if (it != lang_map.end()) {
        return it->second;
    }

    // Default to empty if no mapping found
    return "";
}

std::vector<std::string> WorkflowGenerator::get_linters_for_language(
    const std::string& language,
    const std::string& scan_type) {

    std::vector<std::string> linters;

    // Check if the scan_type exists in the security mappings
    if (!security_mappings_["scan_types"].contains(scan_type)) {
        return linters;
    }

    // Get tools for this scan type from the security mappings
    const auto& scan_type_tools = security_mappings_["scan_types"][scan_type]["tools"];

    // Iterate through all tools for this scan type
    for (auto& [tool_name, tool_info] : scan_type_tools.items()) {
        // Check if this tool supports the requested language
        if (tool_info.contains("supported_languages")) {
            const auto& supported_langs = tool_info["supported_languages"];

            // Check if language is supported by this tool
            bool language_supported = false;
            for (const auto& supported_lang : supported_langs) {
                if (supported_lang.get<std::string>() == language) {
                    language_supported = true;
                    break;
                }
            }

            // Only add the linter if:
            // 1. Language is supported AND
            // 2. "default_for_language" field exists and is set to true
            if (language_supported) {
                // Only add if the field exists and is true
                if (tool_info.contains("default_for_language")) {
                    if (tool_info["default_for_language"].get<bool>()) {
                        auto it = tool_to_linter.find(tool_name);
                        if (it != tool_to_linter.end()) {
                            linters.push_back(it->second);
                        }
                    }
                }
            }
        }
    }

    return linters;
}

std::vector<std::string> WorkflowGenerator::get_generic_linters_for_scan_type(const std::string& scan_type) {
    // Return generic linters that apply to any language for a given scan type
    std::vector<std::string> linters;

    // Map category names from security_mappings.json to scan_types
    static const std::unordered_map<std::string, std::string> category_to_scan_type = {
        {"secret_detection", "security_linting"},
        {"dast", "security_linting"},
        {"license_compliance", "license_compliance"},
        {"runtime_protection", "static_analysis"},
        {"log_analysis", "code_quality"}
    };

    // Look for general recommendations that match this scan type
    for (auto& [category, category_info] : security_mappings_["general_recommendations"].items()) {
        // Skip if this category doesn't map to our scan type
        auto it = category_to_scan_type.find(category);
        if (it == category_to_scan_type.end() || it->second != scan_type) {
            continue;
        }

        // Check if this category has tools
        if (category_info.contains("tools")) {
            // Add tools for this category, but only if they're explicitly marked as default
            for (auto& [tool_name, tool_info] : category_info["tools"].items()) {
                // Check if this tool is marked as default
                bool is_default = false; // Default to false if field doesn't exist

                // Only add if the field exists and is true
                if (tool_info.contains("default_for_language")) {
                    is_default = tool_info["default_for_language"].get<bool>();
                }

                // Only add if it's explicitly marked as a default tool
                if (is_default) {
                    auto tool_it = tool_to_linter.find(tool_name);
                    if (tool_it != tool_to_linter.end()) {
                        linters.push_back(tool_it->second);
                    }
                }
            }
        }
    }

    return linters;
}

void WorkflowGenerator::add_tool_configs(
    std::ofstream& workflow_file,
    const std::string& language,
    const std::string& scan_type) {

    // Map tool names to MegaLinter linter config argument names
    static const std::unordered_map<std::string, std::string> tool_to_config = {
        {"Cppcheck", "CPP_CPPCHECK_ARGUMENTS"},
        {"CppLint", "CPP_CPPLINT_ARGUMENTS"},
        {"clang-tidy", "CPP_CLANG_TIDY_ARGUMENTS"},
        {"Clang Static Analyzer", "CPP_CLANG_TIDY_ARGUMENTS"},
        {"FlawFinder", "CPP_FLAWFINDER_ARGUMENTS"},
        {"Bandit", "PYTHON_BANDIT_ARGUMENTS"},
        {"Pylint", "PYTHON_PYLINT_ARGUMENTS"},
        {"Black", "PYTHON_BLACK_ARGUMENTS"},
        {"Safety", "PYTHON_SAFETY_ARGUMENTS"},
        {"GitLeaks", "REPOSITORY_GITLEAKS_ARGUMENTS"},
        {"Detect-secrets", "REPOSITORY_SECRETLINT_ARGUMENTS"},
        {"Semgrep", "REPOSITORY_SEMGREP_ARGUMENTS"},
        {"CodeQL", "REPOSITORY_CODEQL_ARGUMENTS"},
        {"OWASP Dependency-Check", "REPOSITORY_DEPENDENCY_CHECK_ARGUMENTS"},
        {"Trivy", "REPOSITORY_TRIVY_ARGUMENTS"}
    };

    // Check if the scan_type exists in the security mappings
    if (!security_mappings_["scan_types"].contains(scan_type)) {
        return; // No configuration available
    }

    // Get tools for this scan type from the security mappings
    const auto& scan_type_tools = security_mappings_["scan_types"][scan_type]["tools"];

    // Iterate through all tools for this scan type
    for (auto& [tool_name, tool_info] : scan_type_tools.items()) {
        // Check if this tool supports the requested language
        if (tool_info.contains("supported_languages") && tool_info.contains("configuration")) {
            const auto& supported_langs = tool_info["supported_languages"];

            // Check if language is supported by this tool
            bool language_supported = false;
            for (const auto& supported_lang : supported_langs) {
                if (supported_lang.get<std::string>() == language) {
                    language_supported = true;
                    break;
                }
            }

            // Only configure the tool if:
            // 1. Language is supported AND
            // 2. "default_for_language" field exists and is set to true
            if (language_supported) {
                // Check if this tool is explicitly marked as default for the language
                bool is_default = false; // Default to false if field doesn't exist

                // Only add if the field exists and is true
                if (tool_info.contains("default_for_language")) {
                    is_default = tool_info["default_for_language"].get<bool>();
                }

                // Only add config if it's explicitly marked as a default tool for this language
                if (is_default) {
                    auto it = tool_to_config.find(tool_name);
                    if (it != tool_to_config.end()) {
                        // Extract the configuration string
                        const std::string& config_str = tool_info["configuration"].get<std::string>();

                    // Generate a sensible argument string based on the configuration text
                    std::string args;
                    if (tool_name == "Bandit") {
                        args = "\"-ll -ii\"";
                    } else if (tool_name == "Pylint") {
                        args = "\"--max-line-length=100\"";
                    } else if (tool_name == "Black") {
                        args = "\"--line-length=100\"";
                    } else if (tool_name == "clang-tidy") {
                        args = "\"-checks=clang-analyzer-security.*,cert-*\"";
                    } else if (tool_name == "Cppcheck") {
                        args = "\"--enable=all --inconclusive\"";
                    } else if (tool_name == "Semgrep") {
                        args = "\"--config=p/security-audit\"";
                    } else if (tool_name == "GitLeaks") {
                        args = "\"--config-path=.github/config/gitleaks.toml\"";
                    } else if (tool_name == "Trivy") {
                        args = "\"--severity HIGH,CRITICAL\"";
                    } else {
                        args = "\"\""; // Default to empty arguments
                    }

                    workflow_file << "          " << it->second << ": " << args << "\n";
                }
            }
        }
    }

    // Add generic tool configs from general_recommendations
    if (security_mappings_.contains("general_recommendations")) {
        if (scan_type == "security_linting" &&
            security_mappings_["general_recommendations"].contains("secret_detection")) {
            // Add secret detection tools
            workflow_file << "          REPOSITORY_GITLEAKS_ARGUMENTS: \"--config-path=.github/config/gitleaks.toml\"\n";
        } else if (scan_type == "sast" &&
                  security_mappings_["general_recommendations"].contains("secret_detection")) {
            // Add SAST tools
            workflow_file << "          REPOSITORY_SEMGREP_ARGUMENTS: \"--config=p/security-audit\"\n";
        } else if (scan_type == "cve_scanning" &&
                  (security_mappings_["general_recommendations"].contains("priority_order"))) {
            // Add CVE scanning tools
            workflow_file << "          REPOSITORY_TRIVY_ARGUMENTS: \"--severity HIGH,CRITICAL\"\n";
        }
    }
}

}
}