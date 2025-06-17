# Singularity

# Create build directory

```sh
mkdir build && cd build
```

# Install dependencies with Conan 2.x
```
conan install .. --output-folder=. --build=missing
```

# Configure with CMake using Conan 2.x toolchain
```
cmake -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake ..
```

# Build

```sh
cmake --build .
```

Alternatively, you can use the provided build script:

```bash
# Run the build script (automatically handles Conan setup)
./scripts/build.sh

# Run build script with clang compiler
./scripts/build.sh --clang

# Run build script and execute tests
./scripts/build.sh --test

# Run build script with clang compiler and execute tests
./scripts/build.sh --clang --test
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

### Compiler Support

This project supports both GCC and Clang/LLVM toolchains. By default, it uses the system's default compiler, but you can explicitly use Clang:

```bash
# Build with Clang/LLVM
./scripts/build.sh --clang
```

For convenience, there's also a wrapper script `build_with_clang.sh` that does the same thing.

When using Docker, the build automatically uses Clang as configured in the Dockerfile.

## License

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this software except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
