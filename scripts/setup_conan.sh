#!/bin/bash

set -e

echo "Setting up Conan profiles..."

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

# Conan 2.x required
if [ -z "$MAJOR_VERSION" ] || [ "$MAJOR_VERSION" -lt "2" ]; then
    echo "Error: This project requires Conan 2.x. Please upgrade your Conan installation:"
    echo "pip install --upgrade \"conan>=2.0.0\""
    exit 1
fi

# Create default profile
echo "Creating Conan default profile..."
conan profile detect --force
PROFILE_PATH=$(conan profile path default)
echo "Profile path: $PROFILE_PATH"

echo "Conan profile setup complete."
echo "You can now build the project with: ./scripts/build.sh"
