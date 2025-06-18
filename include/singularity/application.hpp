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

#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>

#include "singularity/language_stats.hpp"

namespace singularity {

/**
 * @brief Command line options parser and application entry point
 */
class Application {
public:
    /**
     * @brief Constructor
     */
    Application();

    /**
     * @brief Run the application with command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return Exit code
     */
    int run(int argc, char** argv);

private:
    /**
     * @brief Parse command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return True if parsing successful and execution should continue
     */
    bool parse_args(int argc, char** argv);

    /**
     * @brief Print application usage
     */
    void print_usage();

    /**
     * @brief Print application version
     */
    void print_version();

    /**
     * @brief Analyze repository and print results
     * @return True if analysis successful
     */
    bool analyze_repo();

    /**
     * @brief Generate security report for the repository
     * @param stats Language statistics from analysis
     * @return True if report generation successful
     */
    bool generate_security_report(const LanguageStats& stats);

    /**
     * @brief Progress callback function
     * @param percentage Progress percentage (0-100)
     * @param message Status message
     */
    void on_progress(int percentage, const std::string& message);

    std::string repo_url_;
    bool verbose_{false};
    bool generate_report_{false};
    std::string output_file_;
};

} // namespace singularity
