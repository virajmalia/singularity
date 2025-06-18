#!/bin/bash

set -e

echo "Setting up Conan 2.x profiles for Singularity project..."

# Check if Conan is installed
if ! command -v conan &> /dev/null; then
    echo "Error: Conan is not installed. Please install Conan before running this script."
    echo "In a virtual environment, you can install it with: pip install conan"
    exit 1
fi

# Check Conan version
CONAN_VERSION=$(conan --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
if [ -z "$CONAN_VERSION" ]; then
    # Try a different pattern if the above fails
    CONAN_VERSION=$(conan --version | grep -oE '[0-9]+\.[0-9]+' | head -1)
fi
MAJOR_VERSION=$(echo $CONAN_VERSION | cut -d. -f1)

echo "Detected Conan version: $CONAN_VERSION"

# Ensure we're using Conan 2.x
if [ -z "$MAJOR_VERSION" ] || [ "$MAJOR_VERSION" -lt "2" ]; then
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
    if CLANG_FULL_VERSION=$(clang --version | grep -oE 'version [0-9]+\.[0-9]+\.[0-9]+' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1); then
        echo "Detected clang full version: $CLANG_FULL_VERSION"
    else
        # Fallback for different version output formats
        CLANG_FULL_VERSION=$(clang --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
        # If still empty, try more formats
        if [ -z "$CLANG_FULL_VERSION" ]; then
            if CLANG_FULL_VERSION=$(clang --version | grep -oE '[0-9]+\.[0-9]+' | head -1); then
                echo "Detected clang version with major.minor format: $CLANG_FULL_VERSION"
            else
                # Try to extract just the major version
                MAJOR_VERSION=$(clang --version | grep -oE 'clang version [0-9]+' | grep -oE '[0-9]+' | head -1)
                CLANG_FULL_VERSION="$MAJOR_VERSION.0.0"  # Just for logging
            fi
        fi
    fi
    
    # Extract just the major version number (first number before the dot)
    CLANG_MAJOR_VERSION=$(echo $CLANG_FULL_VERSION | cut -d. -f1)
    echo "Using clang major version for Conan: $CLANG_MAJOR_VERSION"
    
    # Update compiler version - use only the major version
    sed -i "s/compiler.version=.*/compiler.version=$CLANG_MAJOR_VERSION/g" "$PROFILE_PATH"
    
    # Set libc++ as the C++ standard library
    if grep -q "compiler.libcxx" "$PROFILE_PATH"; then
        sed -i 's/compiler.libcxx=.*/compiler.libcxx=libc++/g' "$PROFILE_PATH"
    else
        echo "compiler.libcxx=libc++" >> "$PROFILE_PATH"
    fi
    
    # Set C++17 as the C++ standard
    if grep -q "compiler.cppstd" "$PROFILE_PATH"; then
        sed -i 's/compiler.cppstd=.*/compiler.cppstd=17/g' "$PROFILE_PATH"
    else
        echo "compiler.cppstd=17" >> "$PROFILE_PATH"
    fi
    
    echo "Conan profile configured successfully!"
    echo "Contents of $PROFILE_PATH:"
    cat "$PROFILE_PATH"
else
    echo "Error: Could not find Conan profile at $PROFILE_PATH"
    exit 1
fi

echo "Conan profile setup complete."
echo "You can now build the project with: ./scripts/build.sh"
