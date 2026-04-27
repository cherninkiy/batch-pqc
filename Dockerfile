FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gcc \
    git \
    libcppunit-dev \
    libntl-dev \
    libssl-dev \
    make \
    pkg-config \
    python3 \
    python3-pip \
    python3-matplotlib \
  \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/batch-pqc

# Copy repository into the image
COPY . /opt/batch-pqc

# Build the project inside the image.
# Note: submodules are not initialized in the image build context. Ensure
# submodules are checked out on the host before building the image (or add
# them to the build context). Attempting to run `git submodule` here will
# usually fail because `.git` is excluded from the Docker build context.
RUN mkdir -p build \
 && cmake -S . -B build \
 && cmake --build build --parallel

CMD ["/bin/bash"]
