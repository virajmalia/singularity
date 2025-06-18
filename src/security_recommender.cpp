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

#include "singularity/security_recommender.hpp"
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

// Simple JSON parsing helpers for our specific use case
namespace {
    class SimpleJsonValue {
    public:
        SimpleJsonValue() : type_(Type::Null) {}
        
        enum class Type { Null, String, Object, Array };
        
        Type getType() const { return type_; }
        
        void setString(const std::string& value) {
            type_ = Type::String;
            string_value_ = value;
        }
        
        void setObject() {
            type_ = Type::Object;
            object_value_.clear();
        }
        
        void setArray() {
            type_ = Type::Array;
            array_value_.clear();
        }
        
        std::string getString() const { return string_value_; }
        
        void addToObject(const std::string& key, const SimpleJsonValue& value) {
            object_value_[key] = value;
        }
        
        void addToArray(const SimpleJsonValue& value) {
            array_value_.push_back(value);
        }
        
        const SimpleJsonValue& operator[](const std::string& key) const {
            static SimpleJsonValue null_value;
            auto it = object_value_.find(key);
            if (it != object_value_.end()) {
                return it->second;
            }
            return null_value;
        }
        
        SimpleJsonValue& operator[](const std::string& key) {
            return object_value_[key];
        }
        
        const std::vector<SimpleJsonValue>& getArray() const {
            return array_value_;
        }
        
        const std::map<std::string, SimpleJsonValue>& getObject() const {
            return object_value_;
        }
        
    private:
        Type type_;
        std::string string_value_;
        std::map<std::string, SimpleJsonValue> object_value_;
        std::vector<SimpleJsonValue> array_value_;
    };
    
    SimpleJsonValue parseJson(std::istream& input) {
        std::string line;
        SimpleJsonValue root;
        root.setObject();
        
        std::string current_section;
        std::map<std::string, SimpleJsonValue> current_language_mappings;
        std::map<std::string, SimpleJsonValue> general_recommendations;
        
        while (std::getline(input, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            if (line.empty() || line[0] == '#' || line[0] == '/') {
                continue; // Skip comments and empty lines
            }
            
            if (line == "{" || line == "}") {
                continue; // Skip brackets
            }
            
            // Look for section headers
            if (line == "\"language_mappings\": {") {
                current_section = "language_mappings";
                SimpleJsonValue mappings;
                mappings.setObject();
                root.addToObject("language_mappings", mappings);
                continue;
            }
            else if (line == "\"general_recommendations\": {") {
                current_section = "general_recommendations";
                SimpleJsonValue recs;
                recs.setObject();
                root.addToObject("general_recommendations", recs);
                continue;
            }
            else if (line == "\"cicd_templates\": {") {
                current_section = "cicd_templates";
                SimpleJsonValue templates;
                templates.setObject();
                root.addToObject("cicd_templates", templates);
                continue;
            }
            
            // Parse key/value pairs
            auto colon_pos = line.find(":");
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                
                // Trim quotes and whitespace
                key.erase(0, key.find_first_not_of(" \t\r\n\""));
                key.erase(key.find_last_not_of(" \t\r\n\"") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n\""));
                value.erase(value.find_last_not_of(" \t\r\n\",") + 1);
                
                if (current_section == "language_mappings") {
                    if (key.find("_tools") != std::string::npos || key.find("_scanners") != std::string::npos) {
                        // This is an array of tools
                        SimpleJsonValue tools;
                        tools.setArray();
                        
                        // Parse the array
                        if (value == "[") {
                            // Multi-line array, read until closing bracket
                            while (std::getline(input, line)) {
                                line.erase(0, line.find_first_not_of(" \t\r\n"));
                                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                                
                                if (line == "]" || line == "],") {
                                    break;
                                }
                                
                                // Parse the tool name
                                line.erase(0, line.find_first_not_of(" \t\r\n\""));
                                line.erase(line.find_last_not_of(" \t\r\n\",") + 1);
                                
                                SimpleJsonValue tool;
                                tool.setString(line);
                                tools.addToArray(tool);
                            }
                        }
                        
                        // Add to the language mapping
                        SimpleJsonValue& lang_obj = root["language_mappings"][current_language_mappings["name"].getString()];
                        if (lang_obj.getType() == SimpleJsonValue::Type::Null) {
                            lang_obj.setObject();
                        }
                        lang_obj.addToObject(key, tools);
                    }
                    else if (key.find("_config") != std::string::npos || key.find("_integration") != std::string::npos || key.find("_metrics") != std::string::npos) {
                        // This is a configuration string
                        SimpleJsonValue config;
                        config.setString(value);
                        
                        // Add to the language mapping
                        SimpleJsonValue& lang_obj = root["language_mappings"][current_language_mappings["name"].getString()];
                        if (lang_obj.getType() == SimpleJsonValue::Type::Null) {
                            lang_obj.setObject();
                        }
                        lang_obj.addToObject(key, config);
                    }
                    else if (key.length() == 1) {
                        // This is a language name
                        SimpleJsonValue lang_name;
                        lang_name.setString(value);
                        current_language_mappings["name"] = lang_name;
                    }
                }
                else if (current_section == "general_recommendations") {
                    if (key.find("_tools") != std::string::npos) {
                        // This is an array of tools
                        SimpleJsonValue tools;
                        tools.setArray();
                        
                        // Parse the array
                        if (value == "[") {
                            // Multi-line array, read until closing bracket
                            while (std::getline(input, line)) {
                                line.erase(0, line.find_first_not_of(" \t\r\n"));
                                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                                
                                if (line == "]" || line == "],") {
                                    break;
                                }
                                
                                // Parse the tool name
                                line.erase(0, line.find_first_not_of(" \t\r\n\""));
                                line.erase(line.find_last_not_of(" \t\r\n\",") + 1);
                                
                                SimpleJsonValue tool;
                                tool.setString(line);
                                tools.addToArray(tool);
                            }
                        }
                        
                        root["general_recommendations"].addToObject(key, tools);
                    }
                    else {
                        // This is a general recommendation string
                        SimpleJsonValue rec;
                        rec.setString(value);
                        root["general_recommendations"].addToObject(key, rec);
                    }
                }
                else if (current_section == "cicd_templates") {
                    SimpleJsonValue template_value;
                    template_value.setString(value);
                    root["cicd_templates"].addToObject(key, template_value);
                }
            }
        }
        
        return root;
    }
}

namespace singularity {

// Helper function to convert SecurityScanType to string
std::string scan_type_to_string(SecurityScanType type) {
    switch (type) {
        case SecurityScanType::STATIC_ANALYSIS:
            return "Static Analysis";
        case SecurityScanType::DEPENDENCY_VULNERABILITY:
            return "Dependency Vulnerability";
        case SecurityScanType::SECRET_DETECTION:
            return "Secret Detection";
        case SecurityScanType::SAST:
            return "SAST";
        case SecurityScanType::DAST:
            return "DAST";
        case SecurityScanType::SECURITY_LINTING:
            return "Security Linting";
        case SecurityScanType::CONTAINER_SECURITY:
            return "Container Security";
        case SecurityScanType::CODE_QUALITY:
            return "Code Quality";
        case SecurityScanType::LICENSE_COMPLIANCE:
            return "License Compliance";
        default:
            return "Unknown";
    }
}

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

        // Parse JSON mappings
        SimpleJsonValue mappings_json = parseJson(mappings_file);

        // Initialize language-specific tools
        const SimpleJsonValue& language_mappings = mappings_json["language_mappings"];
        for (const auto& [language, tools] : language_mappings.getObject()) {
            // Static Analysis
            std::vector<SecurityTool> static_analysis_tools;
            if (tools["static_analysis_tools"].getType() == SimpleJsonValue::Type::Array) {
                for (const auto& tool : tools["static_analysis_tools"].getArray()) {
                    static_analysis_tools.push_back({
                        tool.getString(),
                        "Static analysis tool for " + language,
                        "", // URL would come from a more detailed mapping
                        "", // Installation instructions would come from a more detailed mapping
                        ""  // Configuration guidance would come from a more detailed mapping
                    });
                }
            }
            add_language_tools(language, SecurityScanType::STATIC_ANALYSIS, 
                static_analysis_tools, tools["static_analysis_config"].getString());
            
            // Dependency Vulnerability
            std::vector<SecurityTool> cve_tools;
            if (tools["cve_scanners"].getType() == SimpleJsonValue::Type::Array) {
                for (const auto& tool : tools["cve_scanners"].getArray()) {
                    cve_tools.push_back({
                        tool.getString(),
                        "Dependency vulnerability scanner for " + language,
                        "", // URL
                        "", // Installation
                        ""  // Configuration
                    });
                }
            }
            add_language_tools(language, SecurityScanType::DEPENDENCY_VULNERABILITY, 
                cve_tools, tools["cve_integration"].getString());
            
            // SAST
            std::vector<SecurityTool> sast_tools;
            if (tools["sast_tools"].getType() == SimpleJsonValue::Type::Array) {
                for (const auto& tool : tools["sast_tools"].getArray()) {
                    sast_tools.push_back({
                        tool.getString(),
                        "SAST tool for " + language,
                        "", // URL
                        "", // Installation
                        ""  // Configuration
                    });
                }
            }
            add_language_tools(language, SecurityScanType::SAST, 
                sast_tools, tools["sast_integration"].getString());
            
            // Security Linters
            std::vector<SecurityTool> linter_tools;
            if (tools["security_linters"].getType() == SimpleJsonValue::Type::Array) {
                for (const auto& tool : tools["security_linters"].getArray()) {
                    linter_tools.push_back({
                        tool.getString(),
                        "Security linter for " + language,
                        "", // URL
                        "", // Installation
                        ""  // Configuration
                    });
                }
            }
            add_language_tools(language, SecurityScanType::SECURITY_LINTING, 
                linter_tools, tools["linter_config"].getString());
            
            // Code Quality
            std::vector<SecurityTool> quality_tools;
            if (tools["code_quality_tools"].getType() == SimpleJsonValue::Type::Array) {
                for (const auto& tool : tools["code_quality_tools"].getArray()) {
                    quality_tools.push_back({
                        tool.getString(),
                        "Code quality tool for " + language,
                        "", // URL
                        "", // Installation
                        ""  // Configuration
                    });
                }
            }
            add_language_tools(language, SecurityScanType::CODE_QUALITY, 
                quality_tools, tools["code_quality_metrics"].getString());
        }
        
        // Initialize general tools
        const SimpleJsonValue& general_recs = mappings_json["general_recommendations"];
        
        // Secret Detection
        std::vector<SecurityTool> secret_tools;
        if (general_recs["secret_detection_tools"].getType() == SimpleJsonValue::Type::Array) {
            for (const auto& tool : general_recs["secret_detection_tools"].getArray()) {
                secret_tools.push_back({
                    tool.getString(),
                    "Secret detection tool",
                    "", // URL
                    "", // Installation
                    ""  // Configuration
                });
            }
        }
        add_general_tools(SecurityScanType::SECRET_DETECTION, 
            secret_tools, general_recs["secret_detection_strategy"].getString());
        
        // DAST
        std::vector<SecurityTool> dast_tools;
        if (general_recs["dast_tools"].getType() == SimpleJsonValue::Type::Array) {
            for (const auto& tool : general_recs["dast_tools"].getArray()) {
                dast_tools.push_back({
                    tool.getString(),
                    "Dynamic application security testing tool",
                    "", // URL
                    "", // Installation
                    ""  // Configuration
                });
            }
        }
        add_general_tools(SecurityScanType::DAST, 
            dast_tools, general_recs["dast_strategy"].getString());
        
        // Container Security
        std::vector<SecurityTool> container_tools;
        if (general_recs["container_scanning_tools"].getType() == SimpleJsonValue::Type::Array) {
            for (const auto& tool : general_recs["container_scanning_tools"].getArray()) {
                container_tools.push_back({
                    tool.getString(),
                    "Container security scanning tool",
                    "", // URL
                    "", // Installation
                    ""  // Configuration
                });
            }
        }
        add_general_tools(SecurityScanType::CONTAINER_SECURITY, 
            container_tools, general_recs["container_scanning_methods"].getString());
        
        // License Compliance
        std::vector<SecurityTool> license_tools;
        if (general_recs["license_compliance_tools"].getType() == SimpleJsonValue::Type::Array) {
            for (const auto& tool : general_recs["license_compliance_tools"].getArray()) {
                license_tools.push_back({
                    tool.getString(),
                    "License compliance tool",
                    "", // URL
                    "", // Installation
                    ""  // Configuration
                });
            }
        }
        add_general_tools(SecurityScanType::LICENSE_COMPLIANCE, 
            license_tools, general_recs["license_policy"].getString());

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
        
        const char* env_path = std::getenv("SINGULARITY_TEMPLATE_PATH");
        if (env_path) {
            possible_paths.insert(possible_paths.begin(), env_path);
        }
        
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
        std::string repo_name = "Unknown Repository";
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
        report = std::regex_replace(report, std::regex("\\{\\{REPOSITORY_URL\\}\\}"), "");
        report = std::regex_replace(report, std::regex("\\{\\{TOOL_NAME\\}\\}"), "Singularity Security Analyzer");
        report = std::regex_replace(report, std::regex("\\{\\{CONTACT_INFO\\}\\}"), "support@singularity.example.com");

        // Generate languages detected section
        std::vector<std::string> languages = stats.get_languages();
        std::stringstream languages_ss;
        languages_ss << "The following languages were detected in this repository:\n\n";
        for (const auto& language : languages) {
            double percentage = stats.get_language_percentage(language);
            languages_ss << "- **" << language << "**: " << std::fixed << std::setprecision(1) 
                        << percentage << "% of codebase\n";
        }
        report = std::regex_replace(report, std::regex("\\{\\{LANGUAGES_DETECTED\\}\\}"), languages_ss.str());

        // Process language-specific sections
        std::string language_template_start = "\\{\\{#LANGUAGES\\}\\}";
        std::string language_template_end = "\\{\\{/LANGUAGES\\}\\}";
        
        std::size_t start_pos = report.find("{{#LANGUAGES}}");
        std::size_t end_pos = report.find("{{/LANGUAGES}}");
        
        while (start_pos != std::string::npos && end_pos != std::string::npos) {
            // Extract the language template
            std::string language_template = report.substr(start_pos + 14, end_pos - start_pos - 14);
            
            // Generate content for each language
            std::stringstream language_content;
            for (const auto& language : languages) {
                std::string lang_section = language_template;
                lang_section = std::regex_replace(lang_section, std::regex("\\{\\{LANGUAGE_NAME\\}\\}"), language);
                
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

        // Generate GitHub Actions Workflow
        std::string github_workflow = generate_github_workflow(stats);
        report = std::regex_replace(report, std::regex("\\{\\{GITHUB_ACTIONS_WORKFLOW\\}\\}"), github_workflow);
        
        // Other CI/CD Integration (placeholder)
        std::string other_cicd = "Additional CI/CD integration guides can be provided for GitLab CI, Jenkins, or other platforms upon request.";
        report = std::regex_replace(report, std::regex("\\{\\{OTHER_CICD_INTEGRATION\\}\\}"), other_cicd);
        
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

std::string SecurityRecommender::generate_github_workflow(const LanguageStats& stats) {
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

        // Parse JSON mappings
        SimpleJsonValue mappings_json = parseJson(mappings_file);
        std::string workflow_template = mappings_json["cicd_templates"]["github_actions"].getString();
        
        // Generate language-specific sections
        std::vector<std::string> languages = stats.get_languages();
        std::stringstream language_sections;
        
        for (const auto& language : languages) {
            std::string language_section = workflow_template;
            
            // Replace language placeholders
            language_section = std::regex_replace(language_section, std::regex("\\{\\{LANGUAGE_NAME\\}\\}"), language);
            
            // Generate language-specific workflow steps
            std::string steps = generate_workflow_steps(language);
            language_section = std::regex_replace(language_section, std::regex("\\{\\{LANGUAGE_SPECIFIC_STEPS\\}\\}"), steps);
            
            language_sections << language_section;
        }
        
        // Replace the language template with generated content
        workflow_template = language_sections.str();
        
        return workflow_template;
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

std::string SecurityRecommender::generate_section(SecurityScanType type, const LanguageStats& stats) {
    std::stringstream ss;
    ss << "## " << scan_type_to_string(type) << "\n\n";
    
    // Add general section if applicable
    if (!recommendations_[type].general_tools.empty()) {
        ss << "### General Tools\n\n";
        for (const auto& tool : recommendations_[type].general_tools) {
            ss << "- **" << tool.name << "**: " << tool.description << "\n";
        }
        ss << "\n";
        
        if (!recommendations_[type].general_config.empty()) {
            ss << "**Configuration Guidance**:\n";
            ss << recommendations_[type].general_config << "\n\n";
        }
    }
    
    // Add language-specific sections
    for (const auto& language : stats.get_languages()) {
        auto tools = get_tools_for_language(language, type);
        if (!tools.empty()) {
            ss << generate_language_subsection(language, type);
        }
    }
    
    return ss.str();
}

std::string SecurityRecommender::generate_language_subsection(
    const std::string& language, SecurityScanType scan_type) {
    
    std::stringstream ss;
    ss << "### " << language << "\n\n";
    
    auto tools = get_tools_for_language(language, scan_type);
    for (const auto& tool : tools) {
        ss << "- **" << tool.name << "**\n";
    }
    ss << "\n";
    
    auto config = get_config_for_language(language, scan_type);
    if (!config.empty()) {
        ss << "**Configuration**:\n";
        ss << config << "\n\n";
    }
    
    return ss.str();
}

std::string SecurityRecommender::generate_workflow_steps(const std::string& language) {
    std::stringstream ss;
    
    if (language == "C++" || language == "C") {
        ss << "      - name: Install dependencies\n"
              "        run: sudo apt-get update && sudo apt-get install -y cppcheck clang-tidy\n\n"
              "      - name: Run Cppcheck\n"
              "        run: cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem .\n\n"
              "      - name: Run CodeQL Analysis\n"
              "        uses: github/codeql-action/analyze@v2\n"
              "        with:\n"
              "          languages: cpp\n";
    } else if (language == "Python") {
        ss << "      - name: Set up Python\n"
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
        ss << "      - name: Set up Node.js\n"
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
        ss << "      - name: Set up JDK\n"
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
        ss << "      - name: Set up Go\n"
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
        
        ss << "      - name: Run general security checks\n"
              "        run: echo \"Running security checks for " << language << "\"\n\n"
              "      - name: CodeQL Analysis\n"
              "        uses: github/codeql-action/analyze@v2\n"
              "        with:\n"
              "          languages: " << lowercase_lang << "\n";
    }
    
    return ss.str();
}

} // namespace singularity
