cmake_minimum_required(VERSION 3.15)
project(singularity VERSION 0.1.0 LANGUAGES CXX)

# Set Ninja as preferred generator if available
if(NOT CMAKE_GENERATOR)
    find_program(NINJA ninja)
    if(NINJA)
        set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "")
    endif()
endif()

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options for code quality tools
option(ENABLE_CLANG_FORMAT "Enable clang-format target" OFF)
option(ENABLE_CLANG_TIDY "Enable clang-tidy checks during build" OFF)

# Find dependencies - Conan 2.x with CMakeDeps
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    message(STATUS "Using Conan 2.x toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    
    # Load generated CMake find_package configs
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")
else()
    message(WARNING "Conan toolchain file not found. Dependencies may not be properly linked.")
endif()

# Add cmake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Sources
add_subdirectory(src)

# Testing
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install targets
include(GNUInstallDirs)
install(TARGETS singularity
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Packaging
set(CPACK_PACKAGE_VENDOR "SingularityDev")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool to analyze programming languages used in Git repositories")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
# Add NOTICE file to package
set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE")
# Set license type to Apache 2.0
set(CPACK_PACKAGE_LICENSE "Apache-2.0")
include(CPack)

# clang-format setup
if(ENABLE_CLANG_FORMAT)
    # Find clang-format
    find_program(CLANG_FORMAT_BIN clang-format)
    
    if(CLANG_FORMAT_BIN)
        # Define the list of source files
        file(GLOB_RECURSE ALL_SOURCE_FILES 
            "${CMAKE_SOURCE_DIR}/src/*.cpp"
            "${CMAKE_SOURCE_DIR}/include/*.hpp"
            "${CMAKE_SOURCE_DIR}/tests/*.cpp"
        )
        
        add_custom_target(
            format
            COMMENT "Running clang-format to change files"
            COMMAND ${CLANG_FORMAT_BIN} -style=file -i ${ALL_SOURCE_FILES}
        )
        
        add_custom_target(
            format-check
            COMMENT "Checking clang-format compliance"
            COMMAND ${CMAKE_COMMAND} -E echo "Checking formatting..."
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/clang-format.stamp
            COMMAND ${CLANG_FORMAT_BIN} -style=file -output-replacements-xml ${ALL_SOURCE_FILES} | grep -q "replacement offset" && exit 1 || exit 0
            DEPENDS ${ALL_SOURCE_FILES}
        )
        
        message(STATUS "clang-format targets added: 'format' and 'format-check'")
    else()
        message(WARNING "clang-format requested but executable not found")
    endif()
endif()

# clang-tidy setup
if(ENABLE_CLANG_TIDY)
    # Find clang-tidy
    find_program(CLANG_TIDY_BIN clang-tidy)
    
    if(CLANG_TIDY_BIN)
        # Set the clang-tidy command line
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN};-format-style=file)
        
        # Add a custom target to run clang-tidy on all source files
        file(GLOB_RECURSE ALL_SOURCE_FILES 
            "${CMAKE_SOURCE_DIR}/src/*.cpp"
            "${CMAKE_SOURCE_DIR}/include/*.hpp"
        )
        
        add_custom_target(
            tidy
            COMMENT "Running clang-tidy on all sources"
            COMMAND ${CLANG_TIDY_BIN} -p=${CMAKE_BINARY_DIR} ${ALL_SOURCE_FILES}
        )
        
        message(STATUS "clang-tidy integrated with build and 'tidy' target added")
    else()
        message(WARNING "clang-tidy requested but executable not found")
    endif()
endif()
