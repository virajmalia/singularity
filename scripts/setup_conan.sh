#!/bin/bash

set -e

echo "Setting up Conan 2.x profiles for Singularity project..."

# Check if Conan is installed
if ! command -v conan &> /dev/null; then
    echo "Conan is not installed. Installing now..."
    pip install "conan>=2.0.0"
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
echo "Creating Conan default profile..."
conan profile detect --force
    
    # For Conan 2.x, need to check if compiler is clang and update manually
    # We'll create a temporary file to examine the profile content
    PROFILE_PATH=$(conan profile path default)
    echo "Profile path: $PROFILE_PATH"
    
    if [ -f "$PROFILE_PATH" ]; then
        # Check if compiler is gcc
        if grep -q "compiler=gcc" "$PROFILE_PATH"; then
            echo "GCC compiler detected, setting libstdc++11..."
            # Use sed to update the libcxx setting
            if grep -q "compiler.libcxx" "$PROFILE_PATH"; then
                sed -i 's/compiler.libcxx=.*/compiler.libcxx=libstdc++11/g' "$PROFILE_PATH"
            else
                echo "compiler.libcxx=libstdc++11" >> "$PROFILE_PATH"
            fi
        # Check if compiler is clang
        elif grep -q "compiler=clang" "$PROFILE_PATH" || grep -q "compiler=apple-clang" "$PROFILE_PATH"; then
            echo "Clang compiler detected, setting libc++..."
            # Use sed to update the libcxx setting
            if grep -q "compiler.libcxx" "$PROFILE_PATH"; then
                sed -i 's/compiler.libcxx=.*/compiler.libcxx=libc++/g' "$PROFILE_PATH"
            else
                echo "compiler.libcxx=libc++" >> "$PROFILE_PATH"
            fi
        else
            echo "Unable to detect compiler type in profile."
        fi
        
        echo "Updated profile content:"
        cat "$PROFILE_PATH"
    else
        echo "Warning: Could not find Conan profile at $PROFILE_PATH"
    fi

echo "Conan profile setup complete."
echo "You can now build the project with: ./scripts/build.sh"
