#!/bin/bash
# Script for managing third_party dependencies: install, build, test, clean
set -e



## Usage: ./third_party.sh [install|build|tests|all|clear|rebuild|help]
#
# install  - install dependencies and initialize submodules
# build    - build all third_party
# tests    - run all tests (CTest and manual)
# clear    - remove build directory
# rebuild  - clear and build
# all      - full cycle: install, build, tests (default)
# help     - show this help message
ACTION="${1:-all}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
do_clear() {
  echo "[clear] Removing build directory..."
  rm -rf "$BUILD_DIR"
}

do_rebuild() {
  do_clear
  do_build
}

do_install() {
  echo "[1/3] Installing dependencies..."
  sudo apt-get update
  sudo apt-get install -y cmake build-essential pkg-config libssl-dev libcppunit-dev libntl-dev
  echo "[2/3] Initializing/updating git submodules..."
  git submodule sync --recursive
  git submodule update --init --recursive
}

do_build() {
  echo "[3/3] Building all third_party..."
  PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
  cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
  cmake --build "$BUILD_DIR" --parallel
}

do_tests() {
  echo "[4/4] Running tests..."
  cd "$BUILD_DIR"
  ctest --output-on-failure || true
  if [ -x third_party/shipovnik_example ]; then
    echo "Running shipovnik_example..."
    ./third_party/shipovnik_example
  fi
  if [ -x third_party/hypericum_example ]; then
    echo "Running hypericum_example..."
    ./third_party/hypericum_example
  fi
  if [ -x third_party/PQCgenKAT_sign ]; then
    echo "Running PQCgenKAT_sign..."
    ./third_party/PQCgenKAT_sign
  fi
  if [ -x third_party/kryzhovnik_test ]; then
    echo "Running kryzhovnik_test..."
    ./third_party/kryzhovnik_test
  fi
}


case "$ACTION" in
  install)
    do_install
    ;;
  build)
    do_build
    ;;
  tests)
    do_tests
    ;;
  clear)
    do_clear
    ;;
  rebuild)
    do_rebuild
    ;;
  all)
    do_install
    do_build
    do_tests
    ;;
  help)
    grep '^#' "$0" | sed 's/^# \{0,1\}//'
    exit 0
    ;;
  *)
    echo "Usage: $0 [install|build|tests|all|clear|rebuild|help]"
    exit 1
    ;;
esac

