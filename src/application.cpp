#include "singularity/application.hpp"
#include "singularity/repo_analyzer.hpp"
#include "singularity/formatter.hpp"
#include "singularity/security_recommender.hpp"
#include <cstdlib>  // For std::exit
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#define SINGULARITY_VERSION "0.1.0"

namespace singularity {

Application::Application() = default;

int Application::run(int argc, char** argv) {
    try {
        // Parse command line arguments
        if (!parse_args(argc, argv)) {
            return 0;  // Help or version was shown
        }

        // Analyze repository
        if (!analyze_repo()) {
            return 1;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

// A simple helper function to parse boolean flags
bool parse_bool_flag(const std::string& arg) {
    return arg == "true" || arg == "1" || arg == "yes" || arg == "y" || arg.empty();
}

bool Application::parse_args(int argc, char** argv) {
    // Initialize defaults
    use_api_ = true;
    verbose_ = false;
    generate_report_ = false;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                print_usage();
                return false;
            }
            else if (arg == "-v" || arg == "--version") {
                print_version();
                return false;
            }
            else if (arg == "-r" || arg == "--repo") {
                if (i + 1 < argc) {
                    repo_url_ = argv[++i];
                } else {
                    std::cerr << "Error: --repo requires a URL" << std::endl;
                    print_usage();
                    return false;
                }
            }
            else if (arg == "-p" || arg == "--path") {
                if (i + 1 < argc) {
                    repo_path_ = argv[++i];
                } else {
                    std::cerr << "Error: --path requires a directory path" << std::endl;
                    print_usage();
                    return false;
                }
            }
            else if (arg == "-a" || arg == "--api") {
                if (i + 1 < argc && argv[i+1][0] != '-') {
                    // Next arg is a value, not another flag
                    use_api_ = parse_bool_flag(argv[++i]);
                } else {
                    // Flag without value, assume true
                    use_api_ = true;
                }
            }
            else if (arg == "--security-report") {
                generate_report_ = true;

                // Check if the next argument is an output file path
                if (i + 1 < argc && argv[i+1][0] != '-') {
                    output_file_ = argv[++i];
                }
            }
            else if (arg == "--verbose") {
                verbose_ = true;
            }
            else {
                std::cerr << "Error: Unknown argument: " << arg << std::endl;
                print_usage();
                return false;
            }
        }

        // Check that we have either repo URL or path
        if (repo_url_.empty() && repo_path_.empty()) {
            std::cerr << "Error: Must specify either --repo or --path" << std::endl;
            print_usage();
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        print_usage();
        return false;
    }
}

void Application::print_usage() {
    std::cout << "Singularity - Git repository language detector\n\n"
              << "Usage: singularity --repo <url> [OPTIONS]\n"
              << "   or: singularity --path <local_path> [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help              Show this help message and exit\n"
              << "  -v, --version           Show version and exit\n"
              << "  -r, --repo <url>        URL of git repository to analyze\n"
              << "  -p, --path <path>       Path to local git repository\n"
              << "  -a, --api               Use GitHub API when possible (default: true)\n"
              << "  --verbose               Show verbose output\n"
              << "  --security-report        Generate a security report\n";
}

void Application::print_version() {
    std::cout << "Singularity version " << SINGULARITY_VERSION << "\n";
}

bool Application::analyze_repo() {
    std::unique_ptr<RepoAnalyzer> analyzer;

    // Create analyzer based on input type
    if (!repo_path_.empty()) {
        if (verbose_) {
            std::cout << "Analyzing local repository: " << repo_path_ << std::endl;
        }
        analyzer = RepoAnalyzerFactory::create_local_analyzer(repo_path_);
    } else {
        if (verbose_) {
            std::cout << "Analyzing remote repository: " << repo_url_ << std::endl;
        }
        analyzer = RepoAnalyzerFactory::create_remote_analyzer(repo_url_, use_api_);
    }

    // Set progress callback
    analyzer->set_progress_callback([this](int percentage, const std::string& message) {
        on_progress(percentage, message);
    });

    // Run the analysis
    if (verbose_) {
        std::cout << "Starting analysis...\n";
    }

    LanguageStats stats = analyzer->analyze();

    // Create formatter
    TextFormatter formatter;

    // Format results and write to stdout
    std::cout << formatter.format(stats) << std::endl;

    // Generate security report if requested
    if (generate_report_) {
        if (verbose_) {
            std::cout << "Generating security recommendations report...\n";
        }

        if (!generate_security_report(stats)) {
            return false;
        }
    }

    return true;
}

void Application::on_progress(int percentage, const std::string& message) {
    if (verbose_) {
        std::cout << "[";
        // Pad percentage to 3 characters
        if (percentage < 10) std::cout << "  ";
        else if (percentage < 100) std::cout << " ";
        std::cout << percentage << "%] " << message << std::endl;
    }
}

bool Application::generate_security_report(const LanguageStats& stats) {
    try {
        // Get singleton instance and initialize it
        SecurityRecommender& recommender = SecurityRecommender::instance();
        recommender.initialize();

        // Generate recommendations based on detected languages
        std::string report = recommender.generate_recommendations(stats);

        // Write report to file or stdout
        if (!output_file_.empty()) {
            std::ofstream ofs(output_file_);
            if (ofs) {
                ofs << report;
                ofs.close();

                if (verbose_) {
                    std::cout << "Security report written to " << output_file_ << std::endl;
                }
            } else {
                std::cerr << "Error: Could not open file " << output_file_ << " for writing" << std::endl;
                return false;
            }
        } else {
            // Output to stdout
            std::cout << report << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error generating security report: " << e.what() << std::endl;
        return false;
    }
}

} // namespace singularity
