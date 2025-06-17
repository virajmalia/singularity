FROM ubuntu:22.04 AS builder

# Install build tools only (dependencies will come from Conan)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3-pip \
    git \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install conan
RUN pip3 install conan

# Setup conan
RUN conan profile detect --force && \
    conan profile update conf.tools.system.package_manager:mode=install default && \
    conan profile update conf.tools.system.package_manager:sudo=True default

# Copy source code
WORKDIR /app
COPY . .

# Build with dynamic linking
RUN mkdir build && cd build && \
    conan install .. --output-folder=. --build=missing && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake .. && \
    cmake --build . -j$(nproc)

# Create runtime image
FROM ubuntu:22.04

# When using Conan with dynamic libraries, we need to copy the libraries
# from the builder stage to the runtime image
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy required runtime libraries from builder stage
COPY --from=builder /app/build/bin/ /app/bin/
COPY --from=builder /app/build/lib/ /app/lib/

# Set library path so that the executable can find the shared libraries
ENV LD_LIBRARY_PATH=/app/lib

WORKDIR /app

# Set entrypoint to use the executable from the copied directory
ENTRYPOINT ["/app/bin/singularity"]
