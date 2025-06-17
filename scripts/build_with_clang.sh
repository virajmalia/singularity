#!/bin/bash

set -e

# Script to build the project with clang/LLVM toolchain
echo "Setting up build with clang/LLVM toolchain..."

# Make sure clang is installed
if ! command -v clang &> /dev/null; then
    echo "Error: clang is not installed. Please install it first."
    exit 1
fi

# Export environment variables to use clang
export CC=clang
export CXX=clang++

# Create and go to the build directory
mkdir -p build
cd build

# Setup Conan with clang
../scripts/setup_conan.sh

# Install dependencies with Conan
echo "Installing dependencies with Conan..."
conan install .. --output-folder=. --build=missing

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ \
      ..

# Build
echo "Building..."
cmake --build . -j$(nproc)

# Run tests if requested
if [ "$1" == "--test" ]; then
    echo "Running tests..."
    ctest --output-on-failure
fi

echo "Build completed successfully!"
