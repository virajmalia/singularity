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
#include <unordered_set>
#include <set>
#include <regex>
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

        // Get all detected languages
        std::vector<std::string> languages = stats.get_languages();

        // Process each scan type from security_mappings.json
        const json& scan_types = security_mappings_["scan_types"];

        for (auto it = scan_types.begin(); it != scan_types.end(); ++it) {
            const std::string& scan_type = it.key();

            if (scan_type == "code_quality") {
                generate_lizard_workflow(languages, output_dir);
                continue;
            }

            if (scan_type == "cve_scanning"){
                //generate_dependabot_workflow(languages, output_dir);
                continue;
            }

            // Generate workflow file for this scan type
            if (!generate_megalinter_workflow(scan_type, languages, output_dir)) {
                std::cerr << "Failed to generate workflow for scan type: " << scan_type << std::endl;
                continue;
            }
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
    std::string workflow_filename = output_dir + "/workflows/" + scan_type + ".yml";
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
    workflow_file << "        uses: actions/checkout@v4\n";
    workflow_file << "        with:\n";
    workflow_file << "          fetch-depth: 0\n\n";
    workflow_file << "      # MegaLinter\n";
    workflow_file << "      - name: MegaLinter " << scan_type << " Scan\n";
    workflow_file << "        id: ml\n";
    workflow_file << "        uses: oxsecurity/megalinter@v8\n";
    workflow_file << "        env:\n";
    workflow_file << "          VALIDATE_ALL_CODEBASE: true\n";
    workflow_file << "          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}\n";

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
    workflow_file << "        uses: actions/upload-artifact@v4\n";
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
                        workflow_file << "          " << it->second << "\n";
                        tool_to_config.erase(it); // Remove to avoid duplicates
                    }
                }
            }
        }
    }
}

bool WorkflowGenerator::generate_lizard_workflow(
    const std::vector<std::string>& languages,
    const std::string& output_dir) {

    // Determine which languages are applicable for Lizard analysis
    std::vector<std::string> lizard_languages;
    std::unordered_map<std::string, std::string> language_extensions;

    // Define mappings for supported languages to file extensions
    language_extensions["C"] = "c";
    language_extensions["C++"] = "cpp,cc,cxx,h,hpp,hxx";
    language_extensions["Python"] = "py";

    // Create a set of supported languages for Lizard
    std::unordered_set<std::string> supported_languages = {"C", "C++", "Python"};

    // Filter languages that are supported by Lizard
    for (const auto& language : languages) {
        if (supported_languages.find(language) != supported_languages.end()) {
            lizard_languages.push_back(language);
        }
    }

    // If no supported languages, skip Lizard workflow
    if (lizard_languages.empty()) {
        std::cout << "No languages supported by Lizard detected. Skipping Lizard workflow." << std::endl;
        return true;
    }

    // Create the GitHub Actions workflow file
    std::string workflow_filename = output_dir + "/workflows/lizard-complexity.yml";
    std::ofstream workflow_file(workflow_filename);

    if (!workflow_file.is_open()) {
        std::cerr << "Could not open file for writing: " << workflow_filename << std::endl;
        return false;
    }

    // Get Lizard configuration from security_mappings if available
    int complexity_threshold = 15; // Default threshold
    std::string lizard_args = "";

    // Get the Lizard configuration from security_mappings.json
    if (security_mappings_["scan_types"].contains("code_quality") &&
        security_mappings_["scan_types"]["code_quality"]["tools"].contains("Lizard")) {

        const auto& lizard_config = security_mappings_["scan_types"]["code_quality"]["tools"]["Lizard"];

        // Extract the configuration string if available
        if (lizard_config.contains("configuration")) {
            std::string config_str = lizard_config["configuration"].get<std::string>();

            // Parse complexity threshold if it's in the configuration
            // Look for numbers after "complexity" in the configuration string
            std::size_t pos = config_str.find("complexity");
            if (pos != std::string::npos) {
                // Extract the substring after "complexity" and search within it
                std::string sub_str = config_str.substr(pos);
                std::regex complexityRegex("\\d+");
                std::smatch match;
                if (std::regex_search(sub_str, match, complexityRegex)) {
                    complexity_threshold = std::stoi(match.str());
                }
            }
        }
    }

    // Create workflow content
    workflow_file << "---\n";
    workflow_file << "# Lizard code complexity analysis workflow\n";
    workflow_file << "name: Lizard Code Complexity Analysis\n\n";
    workflow_file << "on:\n";
    workflow_file << "  push:\n";
    workflow_file << "    branches: [main, master]\n";
    workflow_file << "  pull_request:\n";
    workflow_file << "    branches: [main, master]\n";
    workflow_file << "  workflow_dispatch:\n";
    workflow_file << "    # Allows manual triggering from GitHub UI\n";
    workflow_file << "    inputs:\n";
    workflow_file << "      complexity_threshold:\n";
    workflow_file << "        description: 'Complexity threshold (default: " << complexity_threshold << ")'\n";
    workflow_file << "        required: false\n";
    workflow_file << "        default: '" << complexity_threshold << "'\n";
    workflow_file << "        type: string\n";
    workflow_file << "  schedule:\n";
    workflow_file << "    # Run once per week on Sunday at 00:00 UTC\n";
    workflow_file << "    - cron: '0 0 * * 0'\n\n";
    workflow_file << "jobs:\n";
    workflow_file << "  lizard-analysis:\n";
    workflow_file << "    name: Lizard Code Complexity Analysis\n";
    workflow_file << "    runs-on: ubuntu-latest\n";
    workflow_file << "    steps:\n";
    workflow_file << "      # Checkout code\n";
    workflow_file << "      - name: Checkout Code\n";
    workflow_file << "        uses: actions/checkout@v4\n";
    workflow_file << "        with:\n";
    workflow_file << "          fetch-depth: 0\n\n";

    workflow_file << "      # Set up Python\n";
    workflow_file << "      - name: Set up Python\n";
    workflow_file << "        uses: actions/setup-python@v4\n";
    workflow_file << "        with:\n";
    workflow_file << "          python-version: '3.10'\n\n";

    workflow_file << "      # Install Lizard\n";
    workflow_file << "      - name: Install Lizard\n";
    workflow_file << "        run: git clone --branch 1.17.31 https://github.com/terryyin/lizard.git\n\n";

    // Prepare the file extensions for lizard command
    std::string ext_args = "";
    for (const auto& language : lizard_languages) {
        if (language_extensions.find(language) != language_extensions.end()) {
            std::string exts = language_extensions[language];
            std::istringstream ss(exts);
            std::string ext;

            while (std::getline(ss, ext, ',')) {
                if (!ext_args.empty()) {
                    ext_args += " ";
                }
                ext_args += "--extension ." + ext;
            }
        }
    }

    // Add step to set complexity threshold based on input
    workflow_file << "      # Set complexity threshold\n";
    workflow_file << "      - name: Set complexity threshold\n";
    workflow_file << "        run: |\n";
    workflow_file << "          if [[ \"${{ github.event_name }}\" == \"workflow_dispatch\" && \"${{ github.event.inputs.complexity_threshold }}\" != \"\" ]]; then\n";
    workflow_file << "            echo \"COMPLEXITY_THRESHOLD=${{ github.event.inputs.complexity_threshold }}\" >> $GITHUB_ENV\n";
    workflow_file << "          else\n";
    workflow_file << "            echo \"COMPLEXITY_THRESHOLD=" << complexity_threshold << "\" >> $GITHUB_ENV\n";
    workflow_file << "          fi\n\n";

    // Run Lizard for text report
    workflow_file << "      # Run Lizard analysis with text output\n";
    workflow_file << "      - name: Run Lizard Analysis (Text output)\n";
    workflow_file << "        run: |\n";
    workflow_file << "          mkdir -p lizard-reports\n";
    workflow_file << "          echo \"Using complexity threshold: ${{ env.COMPLEXITY_THRESHOLD }}\"\n";
    workflow_file << "          python lizard/lizard.py --CCN ${{ env.COMPLEXITY_THRESHOLD }} " << ext_args << " --warnings_only > lizard-reports/complexity_report.txt\n";
    workflow_file << "          echo \"Complexity issues found:\" $(grep -c \"has \\d\\+\" lizard-reports/complexity_report.txt || echo \"0\")\n\n";

    // Upload Lizard reports as artifacts
    workflow_file << "      # Upload Lizard reports\n";
    workflow_file << "      - name: Upload Lizard reports\n";
    workflow_file << "        uses: actions/upload-artifact@v4\n";
    workflow_file << "        with:\n";
    workflow_file << "          name: Lizard Complexity Reports\n";
    workflow_file << "          path: lizard-reports/\n\n";

    workflow_file << "      # Check if complexity thresholds were exceeded\n";
    workflow_file << "      - name: Check complexity issues\n";
    workflow_file << "        run: |\n";
    workflow_file << "          if [[ $(grep -c \"has \\d\\+\" lizard-reports/complexity_report.txt || echo \"0\") -gt 0 ]]; then\n";
    workflow_file << "            echo \"::warning::Found functions exceeding cyclomatic complexity threshold of ${{ env.COMPLEXITY_THRESHOLD }}\"\n";
    workflow_file << "            cat lizard-reports/complexity_report.txt\n";
    workflow_file << "            # Uncomment the next line to fail the build on complexity issues\n";
    workflow_file << "            # exit 1\n";
    workflow_file << "          else\n";
    workflow_file << "            echo \"No complexity issues found.\"\n";
    workflow_file << "          fi\n";

    workflow_file.close();
    std::cout << "Generated Lizard workflow file: " << workflow_filename << std::endl;
    return true;
}
}