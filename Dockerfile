FROM ubuntu:24.04 AS builder

# Install build tools and clang/LLVM toolchain
RUN apt-get update && apt-get install -y clang llvm lld libc++-dev cmake ninja-build python3-pip && \
    which clang && clang --version

# Set clang as the default compiler
ENV CC=clang
ENV CXX=clang++
ENV PATH="/usr/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/lib:${LD_LIBRARY_PATH}"

# Install Conan package manager
RUN pip install --no-cache-dir --break-system-packages conan && \
    which conan && conan --version
ENV PATH="/usr/local/bin:${PATH}"

# Copy source code
WORKDIR /app
COPY . .

# Setup Conan with clang profile
RUN bash ./scripts/setup_conan.sh

# Build with dynamic linking using clang
RUN mkdir build && cd build && \
    conan install .. --output-folder=. --build=missing && \
    ls && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./conan_toolchain.cmake && \
    cmake --build . -j$(nproc)

# Install application to a specific directory
RUN cd build && cmake --install . --prefix=/app/install

# Set library path so that the executable can find the shared libraries
ENV LD_LIBRARY_PATH=/app/install/lib:${LD_LIBRARY_PATH}

# Set entrypoint to use the executable from the copied directory
ENTRYPOINT ["/bin/bash", "-c", "source /app/build/conanrunenv-release-x86_64.sh && /app/install/bin/singularity --repo https://github.com/virajmalia/singularity"]
