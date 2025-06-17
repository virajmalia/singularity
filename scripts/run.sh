#!/bin/bash

set -e

# Check if the singularity binary exists
if [ ! -f "build/bin/singularity" ]; then
    echo "Singularity binary not found. Building first..."
    ./scripts/build.sh
fi

# Run the application with all arguments passed to this script
./build/bin/singularity "$@"
