#include "singularity/application.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    try {
        singularity::Application app;
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
