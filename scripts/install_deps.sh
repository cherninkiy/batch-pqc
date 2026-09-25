#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
    if command -v sudo >/dev/null 2>&1; then
        exec sudo "$0" "$@"
    fi
    echo "This script requires root privileges or sudo." >&2
    exit 1
fi

export DEBIAN_FRONTEND=noninteractive

APT_PACKAGES=(
    build-essential
    cmake
    gcc
    git
    libcppunit-dev
    libntl-dev
    libssl-dev
    make
    pkg-config
    python3
    python3-matplotlib
    
)

echo "[deps] apt-get update"
apt-get update

echo "[deps] installing: ${APT_PACKAGES[*]}"
apt-get install -y "${APT_PACKAGES[@]}"

echo "[deps] done"
