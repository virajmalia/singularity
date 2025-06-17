#!/bin/bash

# This is a simple wrapper script that calls the main build.sh with the --clang option
# for convenience and backward compatibility

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Forward all arguments to build.sh with --clang added
"${SCRIPT_DIR}/build.sh" --clang "$@"
