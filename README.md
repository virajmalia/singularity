# Singularity

# Create build directory
mkdir build && cd build

# Install dependencies with Conan 2.x
conan install .. --output-folder=. --build=missing

# Configure with CMake using Conan 2.x toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake ..

# Build
cmake --build .
```tps://github.com/username/singularity/actions/workflows/build.yml/badge.svg)](https://github.com/username/singularity/actions/workflows/build.yml)

A simple C++ command-line tool that identifies programming languages used in Git repositories. Singularity scans repository files and provides a clean list of languages detected.

## Features

- Analyze local or remote Git repositories
- Identify languages based on file extensions and patterns
- Print a simple list of detected languages
- Support for both GitHub API analysis and direct repository scanning
- Docker support for containerized execution

## Requirements

- C++17 compatible compiler (Clang recommended)
- CMake 3.15+
- Conan 2.x package manager
- Git

## Building

```bash
# Create build directory
mkdir build && cd build

# Install dependencies with Conan 2.x
conan install .. --output-folder=. --build=missing

# Configure with CMake
cmake ..

# Build
cmake --build .
```

Alternatively, you can use the provided build script:

```bash
# Run the build script (automatically handles Conan setup)
./scripts/build.sh

# Run build script and execute tests
./scripts/build.sh --test
```

## Troubleshooting

### Conan 2.x Profiles

This project requires Conan 2.x. If you encounter errors related to Conan profiles, you can run the setup script:

```bash
./scripts/setup_conan.sh
```

This will create the default Conan 2.x profile with appropriate settings for your compiler.

If you're using an older version of Conan 1.x, you'll need to upgrade:

```bash
pip install --upgrade "conan>=2.0.0"
```

### Build Failures

If you encounter build failures related to missing dependencies, make sure you have:

1. Installed Conan 2.x package manager
2. Run the Conan install step with `--output-folder=.` and `--build=missing` flags
3. Ensured you have a C++17 compatible compiler

## Usage

```bash
# Analyze a remote repository
singularity --repo https://github.com/username/repo

# Analyze a local repository
singularity --path /path/to/local/repo

# Get help
singularity --help
```

## License

MIT
