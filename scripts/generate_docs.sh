#!/bin/bash

set -e

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Doxygen is not installed. Please install it first."
    exit 1
fi

# Run doxygen
doxygen Doxyfile

echo "Documentation generated in doc/html directory."
echo "Open doc/html/index.html in a browser to view it."
