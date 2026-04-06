#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
OUTPUT_DIR="${ROOT_DIR}/results/bench-$(date +%Y%m%d-%H%M%S)"
ALGOS="all"
PARAMS="m_128_20"
BATCH_SIZES="1,2,4,8,16,32,64"
ITERS=100
MSG_SIZE=1024
SEED=1
VERIFY=1
DRY_RUN=0
SKIP_EXISTING=1

usage() {
    cat <<EOF
Usage: ./scripts/benchmark.sh [options]
    --algo <all|shipovnik|hypericum|kryzhovnik>
    --params 
        hypericum: <b_256_64|m_256_64|...> (default: m_128_20)
        shipovnik: <not used>
        kryzhovnik: <not used>
        
  --batch-sizes <comma-separated list>   (default: 1,2,4,8,16,32,64)
  --iters <n>                             (default: 100)
  --msg-size <n>                          (default: 1024)
  --seed <n>                              (default: 1)
  --verify <0|1>                          (default: 1)
  --build-dir <path>                      (default: ./build)
  --output-dir <path>                     (default: ./results/bench-<ts>)
  --dry-run
    --skip-existing <0|1>                   (default: 1)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --algo)
            ALGOS="$2"
            shift 2
            ;;
        --batch-sizes)
            BATCH_SIZES="$2"
            shift 2
            ;;
        --params)
            PARAMS="$2"
            shift 2
            ;;
        --iters)
            ITERS="$2"
            shift 2
            ;;
        --msg-size)
            MSG_SIZE="$2"
            shift 2
            ;;
        --seed)
            SEED="$2"
            shift 2
            ;;
        --verify)
            VERIFY="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --skip-existing)
            SKIP_EXISTING="$2"
            shift 2
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

mkdir -p "$OUTPUT_DIR"

echo "[benchmark] root:      $ROOT_DIR"
echo "[benchmark] build dir: $BUILD_DIR"
echo "[benchmark] output:    $OUTPUT_DIR"
echo "[benchmark] params:    $PARAMS"
echo "[benchmark] skip:      $SKIP_EXISTING"

auto_configure_build() {
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DHYPERICUM_PARAMSET=$PARAMS
    cmake --build "$BUILD_DIR" --parallel
}

run_one() {
    local algo="$1"
    local batch_size="$2"
    local csv_out="$OUTPUT_DIR/${algo}_b${batch_size}.csv"
    local json_out="$OUTPUT_DIR/${algo}_b${batch_size}.json"
    local csv_tmp="$csv_out.tmp"
    local json_tmp="$json_out.tmp"

    if [[ "$SKIP_EXISTING" -eq 1 ]]; then
        if [[ -s "$csv_out" && -s "$json_out" ]]; then
            echo "[skip] algo=$algo batch_size=$batch_size (artifacts exist)"
            return
        fi
    fi

    rm -f "$csv_tmp" "$json_tmp"
    if [[ -f "$csv_out" && ! -s "$csv_out" ]]; then
        rm -f "$csv_out"
    fi
    if [[ -f "$json_out" && ! -s "$json_out" ]]; then
        rm -f "$json_out"
    fi

    local cmd=(
        "$BUILD_DIR/bench/bench_seq"
        --algo "$algo"
        --batch-size "$batch_size"
        --iters "$ITERS"
        --msg-size "$MSG_SIZE"
        --seed "$SEED"
        --params "$PARAMS"
        --verify "$VERIFY"
        --out-csv "$csv_tmp"
        --out-json "$json_tmp"
    )

    if [[ "$DRY_RUN" -eq 1 ]]; then
        echo "[dry-run] ${cmd[*]}"
    else
        echo "[run] algo=$algo batch_size=$batch_size"
        "${cmd[@]}"
        mv "$csv_tmp" "$csv_out"
        mv "$json_tmp" "$json_out"
    fi
}

if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "[dry-run] cmake -S $ROOT_DIR -B $BUILD_DIR -DHYPERICUM_PARAMSET=$PARAMS"
    echo "[dry-run] cmake --build $BUILD_DIR --parallel"
else
    auto_configure_build
fi

IFS=',' read -r -a batches <<< "$BATCH_SIZES"

if [[ "$ALGOS" == "all" ]]; then
    for algo in shipovnik hypericum kryzhovnik; do
        for b in "${batches[@]}"; do
            run_one "$algo" "$b"
        done
    done
else
    for b in "${batches[@]}"; do
        run_one "$ALGOS" "$b"
    done
fi

echo "[benchmark] done"
