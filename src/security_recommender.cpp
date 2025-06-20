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
#include <chrono>
#include <iomanip>
#include <cstdlib>  // For getenv()
#include <fmt/core.h>
#include <regex>
#include <nlohmann/json.hpp>

#include "singularity/security_recommender.hpp"

// Using the nlohmann json library for JSON parsing
using json = nlohmann::json;

namespace singularity {

// Singleton implementation
SecurityRecommender& SecurityRecommender::instance() {
    static SecurityRecommender instance;
    return instance;
}

SecurityRecommender::SecurityRecommender() {
    // Private constructor for singleton
}

void SecurityRecommender::initialize() {
    if (initialized_) {
        return;
    }

    try {
        // Try multiple locations for security mappings file
        std::ifstream mappings_file;
        std::vector<std::string> possible_paths = {
            "security_mappings.json",
            "../security_mappings.json",
            "../../security_mappings.json",
            "../../../security_mappings.json"
        };

        const char* env_path = std::getenv("SINGULARITY_MAPPINGS_PATH");
        if (env_path) {
            possible_paths.insert(possible_paths.begin(), env_path);
        }

        for (const auto& path : possible_paths) {
            mappings_file.open(path);
            if (mappings_file.is_open()) {
                break;
            }
        }

        if (!mappings_file.is_open()) {
            throw std::runtime_error("Could not open security_mappings.json from any expected location");
        }

        // Parse JSON mappings using nlohmann_json
        json mappings_json = nlohmann::json::parse(mappings_file);

        // Initialize scan type-specific tools with language mappings
        const json& scan_types = mappings_json["scan_types"];

        // Process static analysis tools
        if (scan_types.contains("static_analysis") && scan_types["static_analysis"].contains("tools")) {
            const json& static_tools = scan_types["static_analysis"]["tools"];

            // Create a map of language -> tools for static analysis
            std::unordered_map<std::string, std::vector<SecurityTool>> language_static_tools;
            std::unordered_map<std::string, std::string> language_static_configs;

            // Process each tool and its supported languages
            for (const auto& [tool_name, tool_info] : static_tools.items()) {
                if (tool_info.contains("supported_languages") && tool_info["supported_languages"].is_array()) {
                    SecurityTool tool = {
                        tool_name,
                        "Static analysis tool",
                        "", // URL
                        "", // Installation instructions
                        tool_info.value("configuration", "") // Configuration guidance
                    };

                    // Add this tool to each supported language's tools list
                    for (const auto& lang : tool_info["supported_languages"]) {
                        std::string language = lang.get<std::string>();
                        language_static_tools[language].push_back(tool);

                        // Set the configuration if not already set
                        if (language_static_configs.find(language) == language_static_configs.end()) {
                            language_static_configs[language] = tool.configuration;
                        }
                    }
                }
            }

            // Add the tools for each language
            for (const auto& [language, tools] : language_static_tools) {
                add_language_tools(language, SecurityScanType::STATIC_ANALYSIS,
                    tools, language_static_configs[language]);
            }
        }

        // Process dependency vulnerability (CVE) tools
        if (scan_types.contains("cve_scanning") && scan_types["cve_scanning"].contains("tools")) {
            const json& cve_tools_json = scan_types["cve_scanning"]["tools"];

            // Create a map of language -> tools for CVE scanning
            std::unordered_map<std::string, std::vector<SecurityTool>> language_cve_tools;
            std::unordered_map<std::string, std::string> language_cve_configs;

            // Process each tool and its supported languages
            for (const auto& [tool_name, tool_info] : cve_tools_json.items()) {
                if (tool_info.contains("supported_languages") && tool_info["supported_languages"].is_array()) {
                    SecurityTool tool = {
                        tool_name,
                        "Dependency vulnerability scanner",
                        "", // URL
                        "", // Installation instructions
                        tool_info.value("configuration", "") // Configuration guidance
                    };

                    // Add this tool to each supported language's tools list
                    for (const auto& lang : tool_info["supported_languages"]) {
                        std::string language = lang.get<std::string>();
                        language_cve_tools[language].push_back(tool);

                        // Set the configuration if not already set
                        if (language_cve_configs.find(language) == language_cve_configs.end()) {
                            language_cve_configs[language] = tool.configuration;
                        }
                    }
                }
            }

            // Add the tools for each language
            for (const auto& [language, tools] : language_cve_tools) {
                add_language_tools(language, SecurityScanType::DEPENDENCY_VULNERABILITY,
                    tools, language_cve_configs[language]);
            }
        }

        // Process SAST tools
        if (scan_types.contains("sast") && scan_types["sast"].contains("tools")) {
            const json& sast_tools_json = scan_types["sast"]["tools"];

            // Create a map of language -> tools for SAST
            std::unordered_map<std::string, std::vector<SecurityTool>> language_sast_tools;
            std::unordered_map<std::string, std::string> language_sast_configs;

            // Process each tool and its supported languages
            for (const auto& [tool_name, tool_info] : sast_tools_json.items()) {
                if (tool_info.contains("supported_languages") && tool_info["supported_languages"].is_array()) {
                    SecurityTool tool = {
                        tool_name,
                        "SAST tool",
                        "", // URL
                        "", // Installation instructions
                        tool_info.value("configuration", "") // Configuration guidance
                    };

                    // Add this tool to each supported language's tools list
                    for (const auto& lang : tool_info["supported_languages"]) {
                        std::string language = lang.get<std::string>();
                        language_sast_tools[language].push_back(tool);

                        // Set the configuration if not already set
                        if (language_sast_configs[language].empty()) {
                            language_sast_configs[language] = tool.configuration;
                        }
                    }
                }
            }

            // Add the tools for each language
            for (const auto& [language, tools] : language_sast_tools) {
                add_language_tools(language, SecurityScanType::SAST,
                    tools, language_sast_configs[language]);
            }
        }

        // Process Security Linting tools
        if (scan_types.contains("security_linting") && scan_types["security_linting"].contains("tools")) {
            const json& linting_tools_json = scan_types["security_linting"]["tools"];

            // Create a map of language -> tools for security linting
            std::unordered_map<std::string, std::vector<SecurityTool>> language_linting_tools;
            std::unordered_map<std::string, std::string> language_linting_configs;

            // Process each tool and its supported languages
            for (const auto& [tool_name, tool_info] : linting_tools_json.items()) {
                if (tool_info.contains("supported_languages") && tool_info["supported_languages"].is_array()) {
                    SecurityTool tool = {
                        tool_name,
                        "Security linter",
                        "", // URL
                        "", // Installation instructions
                        tool_info.value("configuration", "") // Configuration guidance
                    };

                    // Add this tool to each supported language's tools list
                    for (const auto& lang : tool_info["supported_languages"]) {
                        std::string language = lang.get<std::string>();
                        language_linting_tools[language].push_back(tool);

                        // Set the configuration if not already set
                        if (language_linting_configs[language].empty()) {
                            language_linting_configs[language] = tool.configuration;
                        }
                    }
                }
            }

            // Add the tools for each language
            for (const auto& [language, tools] : language_linting_tools) {
                add_language_tools(language, SecurityScanType::SECURITY_LINTING,
                    tools, language_linting_configs[language]);
            }
        }

        // Process Code Quality tools
        if (scan_types.contains("code_quality") && scan_types["code_quality"].contains("tools")) {
            const json& quality_tools_json = scan_types["code_quality"]["tools"];

            // Create a map of language -> tools for code quality
            std::unordered_map<std::string, std::vector<SecurityTool>> language_quality_tools;
            std::unordered_map<std::string, std::string> language_quality_configs;

            // Process each tool and its supported languages
            for (const auto& [tool_name, tool_info] : quality_tools_json.items()) {
                if (tool_info.contains("supported_languages") && tool_info["supported_languages"].is_array()) {
                    SecurityTool tool = {
                        tool_name,
                        "Code quality tool",
                        "", // URL
                        "", // Installation instructions
                        tool_info.value("configuration", "") // Configuration guidance
                    };

                    // Add this tool to each supported language's tools list
                    for (const auto& lang : tool_info["supported_languages"]) {
                        std::string language = lang.get<std::string>();
                        language_quality_tools[language].push_back(tool);

                        // Set the configuration if not already set
                        if (language_quality_configs.find(language) == language_quality_configs.end()) {
                            language_quality_configs[language] = tool.configuration;
                        }
                    }
                }
            }

            // Add the tools for each language
            for (const auto& [language, tools] : language_quality_tools) {
                add_language_tools(language, SecurityScanType::CODE_QUALITY,
                    tools, language_quality_configs[language]);
            }
        }

        // Initialize general recommendations
        const json& general_recs = mappings_json["general_recommendations"];

        // Secret Detection
        std::vector<SecurityTool> secret_tools;
        if (general_recs.contains("secret_detection") && general_recs["secret_detection"].contains("tools")) {
            for (const auto& [tool_name, tool_info] : general_recs["secret_detection"]["tools"].items()) {
                secret_tools.push_back({
                    tool_name,
                    tool_info.value("description", "Secret detection tool"),
                    "", // URL
                    "", // Installation
                    tool_info.value("configuration", "")
                });
            }

            add_general_tools(SecurityScanType::SECRET_DETECTION,
                secret_tools, general_recs["secret_detection"].value("strategy", ""));
        }

        // DAST
        std::vector<SecurityTool> dast_tools;
        if (general_recs.contains("dast") && general_recs["dast"].contains("tools")) {
            for (const auto& [tool_name, tool_info] : general_recs["dast"]["tools"].items()) {
                dast_tools.push_back({
                    tool_name,
                    tool_info.value("description", "Dynamic application security testing tool"),
                    "", // URL
                    "", // Installation
                    tool_info.value("configuration", "")
                });
            }

            add_general_tools(SecurityScanType::DAST,
                dast_tools, general_recs["dast"].value("strategy", ""));
        }

        // Container Security
        std::vector<SecurityTool> container_tools;
        if (general_recs.contains("container_scanning") && general_recs["container_scanning"].contains("tools")) {
            for (const auto& [tool_name, tool_info] : general_recs["container_scanning"]["tools"].items()) {
                container_tools.push_back({
                    tool_name,
                    tool_info.value("description", "Container security scanning tool"),
                    "", // URL
                    "", // Installation
                    tool_info.value("configuration", "")
                });
            }

            add_general_tools(SecurityScanType::CONTAINER_SECURITY,
                container_tools, general_recs["container_scanning"].value("strategy", ""));
        }

        // License Compliance
        std::vector<SecurityTool> license_tools;
        if (general_recs.contains("license_compliance") && general_recs["license_compliance"].contains("tools")) {
            for (const auto& [tool_name, tool_info] : general_recs["license_compliance"]["tools"].items()) {
                license_tools.push_back({
                    tool_name,
                    tool_info.value("description", "License compliance tool"),
                    "", // URL
                    "", // Installation
                    tool_info.value("configuration", "")
                });
            }

            add_general_tools(SecurityScanType::LICENSE_COMPLIANCE,
                license_tools, general_recs["license_compliance"].value("policy", ""));
        }

        initialized_ = true;
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("Failed to initialize security recommendations: {}", e.what()));
    }
}

std::string SecurityRecommender::generate_recommendations(const LanguageStats& stats) {
    if (!initialized_) {
        initialize();
    }

    try {
        // Try multiple locations for template file
        std::ifstream template_file;
        std::vector<std::string> possible_paths = {
            "security_report_template.md",
            "../security_report_template.md",
            "../../security_report_template.md",
            "../../../security_report_template.md"
        };

        for (const auto& path : possible_paths) {
            template_file.open(path);
            if (template_file.is_open()) {
                break;
            }
        }

        if (!template_file.is_open()) {
            throw std::runtime_error("Could not open security_report_template.md from any expected location");
        }

        std::stringstream template_content;
        template_content << template_file.rdbuf();
        std::string report = template_content.str();

        // Get repository name from stats or use a default
        std::string repo_name = stats.get_repo_name();
        // If we had repository metadata, we would set repo_name here

        // Set basic report information
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream date_ss;
        date_ss << std::put_time(std::localtime(&time_t_now), "%B %d, %Y");
        std::string date_str = date_ss.str();

        report = std::regex_replace(report, std::regex("\\{\\{PROJECT_NAME\\}\\}"), repo_name);
        report = std::regex_replace(report, std::regex("\\{\\{REPORT_DATE\\}\\}"), date_str);
        report = std::regex_replace(report, std::regex("\\{\\{ANALYSIS_VERSION\\}\\}"), "1.0.0");
        report = std::regex_replace(report, std::regex("\\{\\{REPOSITORY_URL\\}\\}"), stats.get_repo_url());
        report = std::regex_replace(report, std::regex("\\{\\{TOOL_NAME\\}\\}"), "Singularity Security Analyzer");
        report = std::regex_replace(report, std::regex("\\{\\{CONTACT_INFO\\}\\}"), "support@singularity.example.com");

        // Generate languages detected section
        std::vector<std::string> languages = stats.get_languages();
        std::stringstream languages_ss;
        languages_ss << "The following languages were detected in this repository:\n\n";
        for (const auto& language : languages) {
            double percentage = stats.get_language_percentage(language);
            if(percentage < 1.0)    continue; // Skip languages with less than 1% of codebase
            languages_ss << "- **" << language << "**: " << std::fixed << std::setprecision(1)
                        << percentage << "% of codebase\n";
        }
        report = std::regex_replace(report, std::regex("\\{\\{LANGUAGES_DETECTED\\}\\}"), languages_ss.str());

        // Process language-specific sections
        std::string language_template_start = "\\{\\{#LANGUAGES\\}\\}";
        std::string language_template_end = "\\{\\{\\/LANGUAGES\\}\\}";

        std::size_t start_pos = report.find("{{#LANGUAGES}}");
        std::size_t end_pos = report.find("{{/LANGUAGES}}");

        while (start_pos != std::string::npos && end_pos != std::string::npos) {
            // Extract the language template
            std::string language_template = report.substr(start_pos + 14, end_pos - start_pos - 14);

            // Generate content for each language
            std::stringstream language_content;
            for (const auto& language : languages) {
                double percentage = stats.get_language_percentage(language);
                if(percentage < 1.0) {
                    continue; // Skip languages with less than 1% of codebase
                }
                std::string lang_section = language_template;
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{LANGUAGE_NAME\\}\\}"), language);

                // Add language-specific flags for C, C++, and Python
                if (language == "C") {
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{#IS_C\\}\\}"), "");
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{\\/IS_C\\}\\}"), "");
                } else {
                    // Remove C-specific sections
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{#IS_C\\}\\}[\\s\\S]*?\\{\\{\\/IS_C\\}\\}"), "");
                }

                if (language == "C++") {
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{#IS_CPP\\}\\}"), "");
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{\\/IS_CPP\\}\\}"), "");
                } else {
                    // Remove C++-specific sections
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{#IS_CPP\\}\\}[\\s\\S]*?\\{\\{\\/IS_CPP\\}\\}"), "");
                }

                if (language == "Python") {
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{#IS_PYTHON\\}\\}"), "");
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{\\/IS_PYTHON\\}\\}"), "");
                } else {
                    // Remove Python-specific sections
                    lang_section = std::regex_replace(lang_section, std::regex("\\{\\{#IS_PYTHON\\}\\}[\\s\\S]*?\\{\\{\\/IS_PYTHON\\}\\}"), "");
                }

                // Static Analysis Tools
                std::stringstream static_tools_ss;
                auto static_tools = get_tools_for_language(language, SecurityScanType::STATIC_ANALYSIS);
                for (const auto& tool : static_tools) {
                    static_tools_ss << "  - " << tool.name << "\n";
                }
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{STATIC_ANALYSIS_TOOLS\\}\\}"), static_tools_ss.str());

                // Static Analysis Config
                std::string static_config = get_config_for_language(language, SecurityScanType::STATIC_ANALYSIS);
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{STATIC_ANALYSIS_CONFIG\\}\\}"), static_config);

                // CVE Scanners
                std::stringstream cve_tools_ss;
                auto cve_tools = get_tools_for_language(language, SecurityScanType::DEPENDENCY_VULNERABILITY);
                for (const auto& tool : cve_tools) {
                    cve_tools_ss << "  - " << tool.name << "\n";
                }
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{CVE_SCANNERS\\}\\}"), cve_tools_ss.str());

                // CVE Integration
                std::string cve_config = get_config_for_language(language, SecurityScanType::DEPENDENCY_VULNERABILITY);
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{CVE_INTEGRATION\\}\\}"), cve_config);

                // SAST Tools
                std::stringstream sast_tools_ss;
                auto sast_tools = get_tools_for_language(language, SecurityScanType::SAST);
                for (const auto& tool : sast_tools) {
                    sast_tools_ss << "  - " << tool.name << "\n";
                }
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{SAST_TOOLS\\}\\}"), sast_tools_ss.str());

                // SAST Integration
                std::string sast_config = get_config_for_language(language, SecurityScanType::SAST);
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{SAST_INTEGRATION\\}\\}"), sast_config);

                // Security Linters
                std::stringstream linter_tools_ss;
                auto linter_tools = get_tools_for_language(language, SecurityScanType::SECURITY_LINTING);
                for (const auto& tool : linter_tools) {
                    linter_tools_ss << "  - " << tool.name << "\n";
                }
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{SECURITY_LINTERS\\}\\}"), linter_tools_ss.str());

                // Linter Config
                std::string linter_config = get_config_for_language(language, SecurityScanType::SECURITY_LINTING);
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{LINTER_CONFIG\\}\\}"), linter_config);

                // Code Quality Tools
                std::stringstream quality_tools_ss;
                auto quality_tools = get_tools_for_language(language, SecurityScanType::CODE_QUALITY);
                for (const auto& tool : quality_tools) {
                    quality_tools_ss << "  - " << tool.name << "\n";
                }
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{CODE_QUALITY_TOOLS\\}\\}"), quality_tools_ss.str());

                // Code Quality Metrics
                std::string quality_metrics = get_config_for_language(language, SecurityScanType::CODE_QUALITY);
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{CODE_QUALITY_METRICS\\}\\}"), quality_metrics);

                language_content << lang_section;
            }

            // Replace the language template with generated content
            report.replace(start_pos, end_pos + 14 - start_pos, language_content.str());

            // Find next occurrence
            start_pos = report.find("{{#LANGUAGES}}");
            end_pos = report.find("{{/LANGUAGES}}");
        }

        // Process general sections
        // Secret Detection Tools
        std::stringstream secret_tools_ss;
        auto secret_tools = get_general_tools(SecurityScanType::SECRET_DETECTION);
        for (const auto& tool : secret_tools) {
            secret_tools_ss << "  - " << tool.name << "\n";
        }
        report = std::regex_replace(report, std::regex("\\{\\{SECRET_DETECTION_TOOLS\\}\\}"), secret_tools_ss.str());

        // Secret Detection Strategy
        auto secret_config = recommendations_[SecurityScanType::SECRET_DETECTION].general_config;
        report = std::regex_replace(report, std::regex("\\{\\{SECRET_DETECTION_STRATEGY\\}\\}"), secret_config);

        // DAST Tools
        std::stringstream dast_tools_ss;
        auto dast_tools = get_general_tools(SecurityScanType::DAST);
        for (const auto& tool : dast_tools) {
            dast_tools_ss << "  - " << tool.name << "\n";
        }
        report = std::regex_replace(report, std::regex("\\{\\{DAST_TOOLS\\}\\}"), dast_tools_ss.str());

        // DAST Strategy
        auto dast_config = recommendations_[SecurityScanType::DAST].general_config;
        report = std::regex_replace(report, std::regex("\\{\\{DAST_STRATEGY\\}\\}"), dast_config);

        // Container Security Tools
        std::stringstream container_tools_ss;
        auto container_tools = get_general_tools(SecurityScanType::CONTAINER_SECURITY);
        for (const auto& tool : container_tools) {
            container_tools_ss << "  - " << tool.name << "\n";
        }
        report = std::regex_replace(report, std::regex("\\{\\{CONTAINER_SCANNING_TOOLS\\}\\}"), container_tools_ss.str());

        // Container Security Methods
        auto container_config = recommendations_[SecurityScanType::CONTAINER_SECURITY].general_config;
        report = std::regex_replace(report, std::regex("\\{\\{CONTAINER_SCANNING_METHODS\\}\\}"), container_config);

        // License Compliance Tools
        std::stringstream license_tools_ss;
        auto license_tools = get_general_tools(SecurityScanType::LICENSE_COMPLIANCE);
        for (const auto& tool : license_tools) {
            license_tools_ss << "  - " << tool.name << "\n";
        }
        report = std::regex_replace(report, std::regex("\\{\\{LICENSE_COMPLIANCE_TOOLS\\}\\}"), license_tools_ss.str());

        // License Policy
        auto license_policy = recommendations_[SecurityScanType::LICENSE_COMPLIANCE].general_config;
        report = std::regex_replace(report, std::regex("\\{\\{LICENSE_POLICY\\}\\}"), license_policy);

        // Security Monitoring Recommendations
        std::stringstream log_analysis_ss;
        std::vector<std::string> log_tools = {"ELK Stack", "Graylog", "Splunk", "Loki"};
        for (const auto& tool : log_tools){
            log_analysis_ss << tool << ", ";
        }
        std::string log_analysis = log_analysis_ss.str();
        if (!log_analysis.empty()) {
            log_analysis.erase(log_analysis.length() - 2);  // Remove trailing comma and space
        }
        report = std::regex_replace(report, std::regex("\\{\\{LOG_ANALYSIS_TOOLS\\}\\}"), log_analysis);

        std::stringstream runtime_ss;
        std::vector<std::string> runtime_tools = {"AppArmor", "SELinux", "Falco", "RASP tools"};
        for (const auto& tool : runtime_tools){
            runtime_ss << tool << ", ";
        }
        std::string runtime = runtime_ss.str();
        if (!runtime.empty()) {
            runtime.erase(runtime.length() - 2);  // Remove trailing comma and space
        }
        report = std::regex_replace(report, std::regex("\\{\\{RUNTIME_PROTECTION\\}\\}"), runtime);

        std::stringstream anomaly_ss;
        std::vector<std::string> anomaly_tools = {"Prometheus with alerting", "AIOps platforms", "Custom metrics monitoring"};
        for (const auto& tool : anomaly_tools){
            anomaly_ss << tool << ", ";
        }
        std::string anomaly = anomaly_ss.str();
        if (!anomaly.empty()) {
            anomaly.erase(anomaly.length() - 2);  // Remove trailing comma and space
        }
        report = std::regex_replace(report, std::regex("\\{\\{ANOMALY_DETECTION\\}\\}"), anomaly);

        // Implementation Priority
        std::vector<std::string> priorities = {
            "Fix vulnerabilities with CVEs",
            "Secret scanning integration",
            "SAST integration",
            "Dependency scanning",
            "Static code analysis"
        };
        report = std::regex_replace(report, std::regex("\\{\\{PRIORITY_1\\}\\}"), priorities[0]);
        report = std::regex_replace(report, std::regex("\\{\\{PRIORITY_2\\}\\}"), priorities[1]);
        report = std::regex_replace(report, std::regex("\\{\\{PRIORITY_3\\}\\}"), priorities[2]);
        report = std::regex_replace(report, std::regex("\\{\\{PRIORITY_4\\}\\}"), priorities[3]);
        report = std::regex_replace(report, std::regex("\\{\\{PRIORITY_5\\}\\}"), priorities[4]);

        // Project-specific recommendations (placeholder)
        std::string project_specific = "Based on the repository analysis, focus on implementing the recommended security tools for the primary languages detected.";
        report = std::regex_replace(report, std::regex("\\{\\{PROJECT_SPECIFIC_RECOMMENDATIONS\\}\\}"), project_specific);

        return report;
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("Failed to generate security recommendations: {}", e.what()));
    }
}

std::vector<SecurityTool> SecurityRecommender::get_tools_for_language(
    const std::string& language, SecurityScanType scan_type) {

    if (!initialized_) {
        initialize();
    }

    auto& scan_recs = recommendations_[scan_type];
    auto it = scan_recs.language_tools.find(language);

    if (it != scan_recs.language_tools.end()) {
        return it->second;
    } else {
        return {};
    }
}

std::string SecurityRecommender::get_config_for_language(
    const std::string& language, SecurityScanType scan_type) {

    if (!initialized_) {
        initialize();
    }

    auto& scan_recs = recommendations_[scan_type];
    auto it = scan_recs.language_configs.find(language);

    if (it != scan_recs.language_configs.end()) {
        return it->second;
    } else {
        return "";
    }
}

std::vector<SecurityTool> SecurityRecommender::get_general_tools(SecurityScanType scan_type) {
    if (!initialized_) {
        initialize();
    }

    return recommendations_[scan_type].general_tools;
}

std::string SecurityRecommender::generate_github_workflow(const LanguageStats& stats, const std::string& output_dir) {
    if (!initialized_) {
        initialize();
    }

    try {
        // Try multiple locations for security mappings file
        std::ifstream mappings_file;
        std::vector<std::string> possible_paths = {
            "security_mappings.json",
            "../security_mappings.json",
            "../../security_mappings.json",
            "../../../security_mappings.json"
        };

        const char* env_path = std::getenv("SINGULARITY_MAPPINGS_PATH");
        if (env_path) {
            possible_paths.insert(possible_paths.begin(), env_path);
        }

        for (const auto& path : possible_paths) {
            mappings_file.open(path);
            if (mappings_file.is_open()) {
                break;
            }
        }

        if (!mappings_file.is_open()) {
            throw std::runtime_error("Could not open security_mappings.json from any expected location");
        }

        // Parse JSON mappings using nlohmann_json
        json mappings_json = nlohmann::json::parse(mappings_file);
        std::string workflow_template = mappings_json["cicd_templates"]["github_actions"]["full_template"].get<std::string>();

        // Generate language-specific sections
        std::vector<std::string> languages = stats.get_languages();
        std::stringstream language_sections;
        std::stringstream report_summary;
        report_summary << "The following GitHub Actions workflow file has been generated:\n\n";

        // Create the .github/workflows directory if it doesn't exist
        std::string workflows_dir = output_dir.empty() ? ".github/workflows" : output_dir + "/.github/workflows";

        if (!output_dir.empty()) {
            try {
                std::filesystem::create_directories(workflows_dir);
            } catch (const std::exception& e) {
                throw std::runtime_error(fmt::format("Failed to create workflows directory: {}", e.what()));
            }
        }

        // Process the template to generate a complete workflow
        std::string complete_workflow = workflow_template;

        // Replace the {{#LANGUAGES}}...{{/LANGUAGES}} section with language-specific jobs
        std::stringstream language_jobs;
        for (auto& language : languages) {
            double percentage = stats.get_language_percentage(language);
            if (percentage < 1.0) {
                continue; // Skip languages with less than 1% of codebase
            }
            // Extract the language job template
            std::regex language_section_regex("\\{\\{#LANGUAGES\\}\\}([\\s\\S]*?)\\{\\{/LANGUAGES\\}\\}");
            std::smatch matches;
            if (std::regex_search(workflow_template, matches, language_section_regex) && matches.size() > 1) {
                std::string language_job_template = matches[1].str();

                // Replace placeholders in this specific language job
                std::string language_job = language_job_template;
                std::replace(language.begin(), language.end(), ' ', '-');
                std::replace(language.begin(), language.end(), '+', 'p');
                language_job = std::regex_replace(language_job, std::regex("\\{\\{LANGUAGE_NAME\\}\\}"), language);

                // Generate language-specific workflow steps
                std::string steps = generate_workflow_steps(language);
                language_job = std::regex_replace(language_job, std::regex("\\{\\{LANGUAGE_SPECIFIC_STEPS\\}\\}"), steps);

                language_jobs << language_job;
            }
        }

        // Replace the {{#LANGUAGES}}...{{/LANGUAGES}} section with the generated language jobs
        complete_workflow = std::regex_replace(complete_workflow,
            std::regex("\\{\\{#LANGUAGES\\}\\}[\\s\\S]*?\\{\\{/LANGUAGES\\}\\}"),
            language_jobs.str());

        // Write to file if output directory is specified
        if (!output_dir.empty()) {
            std::string file_path = fmt::format("{}/{}", workflows_dir, "security-scan.yml");
            std::ofstream workflow_file(file_path);
            if (workflow_file.is_open()) {
                workflow_file << complete_workflow;
                workflow_file.close();
                report_summary << "- `.github/workflows/security-scan.yml`: Complete security workflow including language-specific checks and general security scans\n";
            } else {
                throw std::runtime_error(fmt::format("Failed to write workflow file: {}", file_path));
            }
        }

        // For the report
        std::string combined_workflows = complete_workflow;

        // If we wrote files, return the summary instead of the workflows
        if (!output_dir.empty()) {
            return report_summary.str();
        }

        return combined_workflows;
    } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("Failed to generate GitHub workflow: {}", e.what()));
    }
}

void SecurityRecommender::add_language_tools(
    const std::string& language,
    SecurityScanType scan_type,
    std::vector<SecurityTool> tools,
    const std::string& config) {

    recommendations_[scan_type].language_tools[language] = std::move(tools);
    recommendations_[scan_type].language_configs[language] = config;
}

void SecurityRecommender::add_general_tools(
    SecurityScanType scan_type,
    std::vector<SecurityTool> tools,
    const std::string& config) {

    recommendations_[scan_type].general_tools = std::move(tools);
    recommendations_[scan_type].general_config = config;
}

std::string SecurityRecommender::generate_workflow_steps(const std::string& language) {
    std::stringstream ss;

    if (language == "C++" || language == "C") {
        ss << "- name: Install dependencies\n"
              "        run: sudo apt-get update && sudo apt-get install -y cppcheck clang-tidy\n\n"
              "      - name: Run Cppcheck\n"
              "        run: cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem .\n\n"
              "      - name: Run CodeQL Analysis\n"
              "        uses: github/codeql-action/analyze@v2\n";
    } else if (language == "Python") {
        ss << "- name: Set up Python\n"
              "        uses: actions/setup-python@v4\n"
              "        with:\n"
              "          python-version: '3.10'\n\n"
              "      - name: Install dependencies\n"
              "        run: |\n"
              "          python -m pip install --upgrade pip\n"
              "          pip install bandit pylint safety\n"
              "          if [ -f requirements.txt ]; then pip install -r requirements.txt; fi\n\n"
              "      - name: Run Bandit\n"
              "        run: bandit -r .\n\n"
              "      - name: Check for dependency vulnerabilities\n"
              "        run: safety check\n";
    } else if (language == "JavaScript") {
        ss << "- name: Set up Node.js\n"
              "        uses: actions/setup-node@v3\n"
              "        with:\n"
              "          node-version: '18'\n\n"
              "      - name: Install dependencies\n"
              "        run: npm install\n\n"
              "      - name: Run ESLint\n"
              "        run: npx eslint .\n\n"
              "      - name: Check for vulnerabilities\n"
              "        run: npm audit\n";
    } else if (language == "Java") {
        ss << "- name: Set up JDK\n"
              "        uses: actions/setup-java@v3\n"
              "        with:\n"
              "          distribution: 'temurin'\n"
              "          java-version: '17'\n\n"
              "      - name: Run SpotBugs\n"
              "        run: |\n"
              "          if [ -f pom.xml ]; then\n"
              "            mvn com.github.spotbugs:spotbugs-maven-plugin:check\n"
              "          elif [ -f build.gradle ]; then\n"
              "            ./gradlew spotbugsMain\n"
              "          fi\n\n"
              "      - name: Check for dependency vulnerabilities\n"
              "        uses: dependency-check/Dependency-Check_Action@main\n"
              "        with:\n"
              "          project: 'test'\n"
              "          path: '.'\n"
              "          format: 'HTML'\n"
              "          out: 'reports'\n";
    } else if (language == "Go") {
        ss << "- name: Set up Go\n"
              "        uses: actions/setup-go@v4\n"
              "        with:\n"
              "          go-version: '>=1.19.0'\n\n"
              "      - name: Run gosec\n"
              "        uses: securego/gosec@master\n"
              "        with:\n"
              "          args: ./...\n\n"
              "      - name: Run govulncheck\n"
              "        run: |\n"
              "          go install golang.org/x/vuln/cmd/govulncheck@latest\n"
              "          govulncheck ./...\n";
    } else {
        // Generic steps for other languages
        // Convert language name to lowercase for CodeQL
        std::string lowercase_lang = language;
        std::transform(lowercase_lang.begin(), lowercase_lang.end(), lowercase_lang.begin(), ::tolower);

        ss << "- name: Run general security checks\n"
              "        run: echo \"Running security checks for " << language << "\"\n\n"
              "      - name: CodeQL Analysis\n"
              "        uses: github/codeql-action/analyze@v2\n";
    }

    return ss.str();
}

} // namespace singularity
