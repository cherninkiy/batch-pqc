#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME="batch-pqc-bench:latest"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RESULTS_DIR="$ROOT_DIR/results"
DOCS_DIR="$ROOT_DIR/docs/figures"
BUILD_VOLUME="batch-pqc_build"

echo "Building Docker image: $IMAGE_NAME"
docker build -t "$IMAGE_NAME" "$ROOT_DIR"

mkdir -p "$RESULTS_DIR" "$DOCS_DIR"

CMD="scripts/third_party.sh build && ./scripts/benchmark.sh ${*:---algo all --batch-sizes 1,2,4,8,16 --iters 100 --verify 1}"

echo "Running benchmark in container (results -> $RESULTS_DIR, docs -> $DOCS_DIR)"
docker run --rm -it \
  -v "$ROOT_DIR":/opt/batch-pqc \
  -v "$BUILD_VOLUME":/opt/batch-pqc/build \
  -v "$RESULTS_DIR":/opt/batch-pqc/results \
  -v "$DOCS_DIR":/opt/batch-pqc/docs/figures \
  -w /opt/batch-pqc \
  "$IMAGE_NAME" /bin/bash -lc "$CMD"
