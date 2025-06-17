#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>

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
     * @brief Progress callback function
     * @param percentage Progress percentage (0-100)
     * @param message Status message
     */
    void on_progress(int percentage, const std::string& message);

    std::string repo_url_;
    std::string repo_path_;
    bool use_api_{true};
    bool verbose_{false};
};

} // namespace singularity
