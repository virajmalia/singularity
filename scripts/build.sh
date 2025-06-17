#!/bin/bash

set -e

# Setup Conan profiles
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Ensuring Conan 2.x profiles are set up correctly..."
bash "${SCRIPT_DIR}/setup_conan.sh"

# Check if build directory exists
if [ ! -d "build" ]; then
    mkdir build
fi

# Change to build directory
cd build

# Install dependencies with Conan 2.x
echo "Installing dependencies with Conan 2.x..."
conan install .. --output-folder=. --build=missing -c tools.system.package_manager:mode=install -c tools.system.package_manager:sudo=True

# No need to generate custom provider with CMakeDeps and CMakeToolchain
echo "Using Conan 2.x CMakeDeps and CMakeToolchain generators..."

# Configure with CMake using Conan 2.x generated toolchain
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake ..

# Build
cmake --build . -- -j$(nproc)

# Run tests if requested
if [ "$1" == "--test" ]; then
    ctest -V
fi

echo "Build completed successfully!"
