#!/bin/bash

set -e

echo "Setting up Conan 2.x profiles for Singularity project..."

# Check if Conan is installed
if ! command -v conan &> /dev/null; then
    echo "Conan is not installed. Installing now..."
    # Check if we're on Alpine (use pip3 instead of pip)
    if [ -f "/etc/alpine-release" ]; then
        pip3 install conan
    else
        pip install conan
    fi
fi

# Check Conan version
CONAN_VERSION=$(conan --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]+' | head -1)
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
    if CLANG_VERSION=$(clang --version | grep -oE 'version [0-9]+\.[0-9]+\.[0-9]+' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1); then
        echo "Detected clang version: $CLANG_VERSION"
    else
        # Fallback for different version output formats
        CLANG_VERSION=$(clang --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
        # If still empty, try to extract just the major version
        if [ -z "$CLANG_VERSION" ]; then
            CLANG_VERSION=$(clang --version | grep -oE 'clang version [0-9]+' | grep -oE '[0-9]+' | head -1)
            if [ -z "$CLANG_VERSION" ] && [ -f "/etc/alpine-release" ]; then
                # Special case for Alpine's clang-20 package
                CLANG_VERSION="20.0.0"
                echo "Alpine Linux detected, assuming Clang version: $CLANG_VERSION"
            else
                CLANG_VERSION="$CLANG_VERSION.0.0"  # Add minor and patch versions
            fi
            echo "Using simplified clang version: $CLANG_VERSION"
        fi
    fi
    
    MAJOR_MINOR=$(echo $CLANG_VERSION | cut -d. -f1-2)
    echo "Using clang version: $MAJOR_MINOR"
    
    # Update compiler version
    sed -i "s/compiler.version=.*/compiler.version=$MAJOR_MINOR/g" "$PROFILE_PATH"
    
    # Set libc++ as the C++ standard library
    if grep -q "compiler.libcxx" "$PROFILE_PATH"; then
        sed -i 's/compiler.libcxx=.*/compiler.libcxx=libc++/g' "$PROFILE_PATH"
    else
        echo "compiler.libcxx=libc++" >> "$PROFILE_PATH"
    fi
    
    # Special handling for Alpine Linux
    if [ -f "/etc/alpine-release" ]; then
        echo "Detected Alpine Linux, adding additional settings..."
        # Ensure appropriate paths and flags are set for Alpine
        if ! grep -q "tools.system.package_manager:mode" "$PROFILE_PATH"; then
            echo "tools.system.package_manager:mode=install" >> "$PROFILE_PATH"
        fi
        
        # Specify C++ ABI to match libc++
        if ! grep -q "compiler.cppstd" "$PROFILE_PATH"; then
            echo "compiler.cppstd=17" >> "$PROFILE_PATH"
        fi
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
