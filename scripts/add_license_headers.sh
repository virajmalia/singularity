#!/bin/bash

# This script adds the Apache 2.0 license header to all source files in the project
# Requires the LICENSE_HEADER file to exist

# Check if LICENSE_HEADER exists
if [ ! -f "LICENSE_HEADER" ]; then
    echo "ERROR: LICENSE_HEADER file not found!"
    exit 1
fi

# Function to add header if it doesn't exist already
add_license_header() {
    local file="$1"
    
    # Check if file already has license header
    if grep -q "Licensed under the Apache License, Version 2.0" "$file"; then
        echo "License header already exists in $file - skipping"
        return
    fi
    
    echo "Adding license header to $file"
    
    # Create temp file
    local temp_file=$(mktemp)
    
    # Add header followed by empty line and original content
    cat LICENSE_HEADER > "$temp_file"
    echo "" >> "$temp_file"
    echo "" >> "$temp_file"
    cat "$file" >> "$temp_file"
    
    # Replace original file
    mv "$temp_file" "$file"
}

# Find all source files and add header
echo "Adding license headers to all source files..."

# C++ source and header files
find src include tests -name "*.cpp" -o -name "*.hpp" | while read file; do
    add_license_header "$file"
done

# CMake files
find . -name "CMakeLists.txt" | while read file; do
    # For CMake files, we need to use CMake comment style
    if ! grep -q "Licensed under the Apache License, Version 2.0" "$file"; then
        echo "Adding license header to $file (CMake style)"
        temp_file=$(mktemp)
        echo "# Copyright 2025 Singularity Contributors" > "$temp_file"
        echo "#" >> "$temp_file"
        echo "# Licensed under the Apache License, Version 2.0 (the \"License\");" >> "$temp_file"
        echo "# you may not use this file except in compliance with the License." >> "$temp_file"
        echo "# You may obtain a copy of the License at" >> "$temp_file"
        echo "#" >> "$temp_file"
        echo "#     http://www.apache.org/licenses/LICENSE-2.0" >> "$temp_file"
        echo "#" >> "$temp_file"
        echo "# Unless required by applicable law or agreed to in writing, software" >> "$temp_file"
        echo "# distributed under the License is distributed on an \"AS IS\" BASIS," >> "$temp_file"
        echo "# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied." >> "$temp_file"
        echo "# See the License for the specific language governing permissions and" >> "$temp_file"
        echo "# limitations under the License." >> "$temp_file"
        echo "" >> "$temp_file"
        cat "$file" >> "$temp_file"
        mv "$temp_file" "$file"
    fi
done

echo "Done!"
