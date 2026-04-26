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

# Initialize submodules (if present) and perform a build
RUN git submodule sync --recursive || true \
 && git submodule update --init --recursive || true \
 && mkdir -p build \
 && cmake -S . -B build \
 && cmake --build build --parallel

CMD ["/bin/bash"]
