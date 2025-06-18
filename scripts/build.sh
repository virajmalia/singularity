#!/bin/bash

set -e

# Command line arguments
USE_CLANG=false

# Process command line options
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --clang)
            USE_CLANG=true
            shift # past argument
            ;;
        --test)
            RUN_TESTS=true
            shift # past argument
            ;;
        *)    # unknown option
            shift # past argument
            ;;
    esac
done

# Setup compiler if using clang
if [ "$USE_CLANG" = true ]; then
    if ! command -v clang &> /dev/null; then
        echo "Error: clang is not installed. Please install it first."
        exit 1
    fi
    echo "Using clang/LLVM toolchain..."
    export CC=clang
    export CXX=clang++
    # Print clang version for debugging
    echo "Clang version: $(clang --version)"
    
    # Configure to use libc++ on Ubuntu
    export CXXFLAGS="${CXXFLAGS} -stdlib=libc++"
    export LDFLAGS="${LDFLAGS} -stdlib=libc++"
else
    echo "Using default system compiler..."
fi

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

# Configure with CMake using Conan 2.x generated toolchain and Ninja generator
if [ "$USE_CLANG" = true ]; then
    # Pass environment variables to ensure consistent compiler flags
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake \
          -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
          -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
          -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
          -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS}" \
          -DCMAKE_MODULE_LINKER_FLAGS="${LDFLAGS}" ..
else
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake ..
fi

# Build with Ninja
ninja

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    ctest -V
fi

echo "Build completed successfully!"
