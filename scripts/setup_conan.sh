#!/bin/bash

set -e

echo "Setting up Conan 2.x profiles for Singularity project..."

# Check if Conan is installed
if ! command -v conan &> /dev/null; then
    echo "Conan is not installed. Installing now..."
    pip install conan
fi

# Check Conan version
CONAN_VERSION=$(conan --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
MAJOR_VERSION=$(echo $CONAN_VERSION | cut -d. -f1)

echo "Detected Conan version: $CONAN_VERSION"

# Ensure we're using Conan 2.x
if [ "$MAJOR_VERSION" -lt "2" ]; then
    echo "Error: This project requires Conan 2.x. Please upgrade your Conan installation:"
    echo "pip install --upgrade \"conan>=2.0.0\""
    exit 1
fi

# Create default profile
echo "Creating Conan default profile with clang..."
# First detect the base profile
conan profile detect --force

# Now customize it to use clang
PROFILE_PATH=$(conan profile path default)
echo "Profile path: $PROFILE_PATH"

if [ -f "$PROFILE_PATH" ]; then
    echo "Configuring profile to use clang compiler with libc++..."
    
    # Update the compiler to clang
    sed -i 's/compiler=gcc/compiler=clang/g' "$PROFILE_PATH"
    
    # Get the current version of clang
    CLANG_VERSION=$(clang --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
    MAJOR_MINOR=$(echo $CLANG_VERSION | cut -d. -f1-2)
    
    # Update compiler version
    sed -i "s/compiler.version=.*/compiler.version=$MAJOR_MINOR/g" "$PROFILE_PATH"
    
    # Set libc++ as the C++ standard library
    if grep -q "compiler.libcxx" "$PROFILE_PATH"; then
        sed -i 's/compiler.libcxx=.*/compiler.libcxx=libc++/g' "$PROFILE_PATH"
    else
        echo "compiler.libcxx=libc++" >> "$PROFILE_PATH"
    fi
    
    # Also create a specific clang profile for direct reference
    echo "Creating dedicated clang profile..."
    cp "$PROFILE_PATH" "$(dirname "$PROFILE_PATH")/clang"
    echo "Clang profile created at: $(dirname "$PROFILE_PATH")/clang"
    
    # Set tools.build:compiler_executables in the profiles
    echo "compiler.cxx=clang++" >> "$PROFILE_PATH"
    echo "compiler.c=clang" >> "$PROFILE_PATH"
    
    else
        echo "Warning: Could not find Conan profile at $PROFILE_PATH"
    fi

echo "Conan profile setup complete."
echo "You can now build the project with: ./scripts/build.sh"
