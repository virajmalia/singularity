FROM ubuntu:22.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3-pip \
    git \
    libssl-dev \
    libcurl4-openssl-dev \
    libgit2-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install conan
RUN pip3 install "conan>=2.0.0"

# Setup conan
RUN conan profile detect --force && \
    conan profile update conf.tools.system.package_manager:mode=install default && \
    conan profile update conf.tools.system.package_manager:sudo=True default

# Copy source code
WORKDIR /app
COPY . .

# Build
RUN mkdir build && cd build && \
    conan install .. --output-folder=. --build=missing && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake .. && \
    cmake --build . -j$(nproc)

# Create runtime image
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    libgit2-1.5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the executable from the builder stage
COPY --from=builder /app/build/bin/singularity /app/singularity

# Set entrypoint
ENTRYPOINT ["/app/singularity"]
