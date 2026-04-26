#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
OUTPUT_DIR="${ROOT_DIR}/results/bench-$(date +%Y%m%d-%H%M%S)"
ALGOS="all"
HYPERICUM_PARAMS="debug"
KRYZHOVNIK_PARAMS="debug"
BATCH_SIZES="1,2,4,8,16,32,64"
ITERS=100
MSG_SIZE=1024
SEED=1
VERIFY=1
DRY_RUN=0
SKIP_EXISTING=1
MODE="all"

find_plot_python() {
    local candidate
    for candidate in python3 /usr/bin/python3; do
        if command -v "$candidate" >/dev/null 2>&1; then
            if "$candidate" -c 'import matplotlib' >/dev/null 2>&1; then
                printf '%s\n' "$candidate"
                return 0
            fi
        fi
    done
    return 1
}

usage() {
    cat <<EOF
Usage: ./scripts/benchmark.sh [options]
    --algo <all|shipovnik|hypericum|kryzhovnik>
    --hypericum-params <b_256_64|m_256_64|b_256_20|m_256_20|b_128_20|m_128_20|debug> (default: debug)
    --kryzhovnik-params <small|medium|large|debug>   (default: debug)
    --params <value>                           (legacy: sets hypericum;
                                               also sets kryzhovnik when value is small|medium|large|debug)
        
  --batch-sizes <comma-separated list>   (default: 1,2,4,8,16,32,64)
  --iters <n>                             (default: 100)
  --msg-size <n>                          (default: 1024)
  --seed <n>                              (default: 1)
  --verify <0|1>                          (default: 1)
    --mode <seq|batch|all>                  (default: all)
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
            HYPERICUM_PARAMS="$2"
            case "$2" in
                small|medium|large|debug)
                    KRYZHOVNIK_PARAMS="$2"
                    ;;
            esac
            shift 2
            ;;
        --hypericum-params)
            HYPERICUM_PARAMS="$2"
            shift 2
            ;;
        --kryzhovnik-params)
            KRYZHOVNIK_PARAMS="$2"
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
        --mode)
            MODE="$2"
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
echo "[benchmark] hypericum: $HYPERICUM_PARAMS"
echo "[benchmark] kryzhovnik:$KRYZHOVNIK_PARAMS"
echo "[benchmark] skip:      $SKIP_EXISTING"
echo "[benchmark] mode:      $MODE"

if [[ "$MODE" != "seq" && "$MODE" != "batch" && "$MODE" != "all" ]]; then
    echo "Unsupported mode: $MODE"
    usage
    exit 1
fi

auto_configure_build() {
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
        -DHYPERICUM_PARAMSET="$HYPERICUM_PARAMS" \
        -DKRYZHOVNIK_PARAMSET="$KRYZHOVNIK_PARAMS"
    cmake --build "$BUILD_DIR" --parallel
}

run_one() {
    local algo="$1"
    local batch_size="$2"
    local kind="$3"
    local bench_bin="bench_seq"
    local csv_out="$OUTPUT_DIR/${algo}_b${batch_size}_${kind}.csv"
    local json_out="$OUTPUT_DIR/${algo}_b${batch_size}_${kind}.json"
    local csv_tmp="$csv_out.tmp"
    local json_tmp="$json_out.tmp"
    local params_label="default"

    if [[ "$kind" == "batch" ]]; then
        bench_bin="bench_batch"
    fi

    case "$algo" in
        hypericum)
            params_label="$HYPERICUM_PARAMS"
            ;;
        kryzhovnik)
            params_label="$KRYZHOVNIK_PARAMS"
            ;;
    esac

    if [[ "$SKIP_EXISTING" -eq 1 ]]; then
        if [[ -s "$csv_out" && -s "$json_out" ]]; then
            echo "[skip] kind=$kind algo=$algo batch_size=$batch_size (artifacts exist)"
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
        "$BUILD_DIR/bench/$bench_bin"
        --algo "$algo"
        --batch-size "$batch_size"
        --iters "$ITERS"
        --msg-size "$MSG_SIZE"
        --seed "$SEED"
        --params "$params_label"
        --verify "$VERIFY"
        --out-csv "$csv_tmp"
        --out-json "$json_tmp"
    )

    if [[ "$DRY_RUN" -eq 1 ]]; then
        echo "[dry-run] ${cmd[*]}"
    else
        echo "[run] kind=$kind algo=$algo batch_size=$batch_size"
        "${cmd[@]}"
        mv "$csv_tmp" "$csv_out"
        mv "$json_tmp" "$json_out"
    fi
}

run_modes() {
    local algo="$1"
    local batch_size="$2"

    if [[ "$MODE" == "seq" || "$MODE" == "all" ]]; then
        run_one "$algo" "$batch_size" "seq"
    fi

    if [[ "$MODE" == "batch" || "$MODE" == "all" ]]; then
        run_one "$algo" "$batch_size" "batch"
    fi
}

if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "[dry-run] cmake -S $ROOT_DIR -B $BUILD_DIR -DHYPERICUM_PARAMSET=$HYPERICUM_PARAMS -DKRYZHOVNIK_PARAMSET=$KRYZHOVNIK_PARAMS"
    echo "[dry-run] cmake --build $BUILD_DIR --parallel"
else
    auto_configure_build
fi

IFS=',' read -r -a batches <<< "$BATCH_SIZES"

if [[ "$ALGOS" == "all" ]]; then
    for algo in shipovnik hypericum kryzhovnik; do
        for b in "${batches[@]}"; do
            run_modes "$algo" "$b"
        done
    done
else
    for b in "${batches[@]}"; do
        run_modes "$ALGOS" "$b"
    done
fi

if PLOT_PYTHON="$(find_plot_python)"; then
    if [[ "$DRY_RUN" -eq 1 ]]; then
        echo "[dry-run] $PLOT_PYTHON $ROOT_DIR/scripts/plot_results.py --results-dir $OUTPUT_DIR"
    else
        "$PLOT_PYTHON" "$ROOT_DIR/scripts/plot_results.py" --results-dir "$OUTPUT_DIR" || \
            echo "[benchmark] warning: plot generation failed"
    fi
else
    echo "[benchmark] warning: no python interpreter with matplotlib found, skipping plot generation"
fi

echo "[benchmark] done"
