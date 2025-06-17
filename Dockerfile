FROM ubuntu:22.04 AS builder

# Install build tools and clang/LLVM toolchain
RUN apt-get update && apt-get install -y libc++-18-dev cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Set clang as the default compiler
ENV CC=clang
ENV CXX=clang++

# Install conan
RUN pip3 install conan

# Setup conan with proper Conan 2.x syntax
RUN conan profile detect --force && \
    mkdir -p /root/.conan2/profiles && \
    PROFILE_PATH=$(conan profile path default) && \
    echo 'tools.system.package_manager:mode=install' >> $PROFILE_PATH && \
    echo 'tools.system.package_manager:sudo=True' >> $PROFILE_PATH

# Copy source code
WORKDIR /app
COPY . .

# Setup Conan with clang profile
RUN bash ./scripts/setup_conan.sh

# Build with dynamic linking using clang
RUN mkdir build && cd build && \
    conan install .. --output-folder=. --build=missing && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. && \
    cmake --build . -j$(nproc)

# Create runtime image
FROM ubuntu:22.04

# Copy required runtime libraries from builder stage
COPY --from=builder /app/build/bin/ /app/bin/
COPY --from=builder /app/build/lib/ /app/lib/

# Set library path so that the executable can find the shared libraries
ENV LD_LIBRARY_PATH=/app/lib

WORKDIR /app

# Set entrypoint to use the executable from the copied directory
ENTRYPOINT ["/app/bin/singularity"]
