#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path
import shutil


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Aggregate seq/batch benchmark results and render plots.")
    parser.add_argument("--results-dir", required=True, help="Directory containing *_seq.csv and *_batch.csv files")
    parser.add_argument("--output-dir", help="Directory for raw_data.csv and plots/ (defaults to results dir)")
    parser.add_argument("--copy-to-docs", action="store_true", help="Copy generated raw_data.csv and plots to the docs figures directory")
    parser.add_argument("--docs-dir", default="docs/figures", help="Destination docs figures directory when --copy-to-docs is used")
    parser.add_argument("--algo", default="all", help="Algorithm to filter, or 'all'")
    return parser.parse_args()


def load_rows(results_dir: Path, suffix: str) -> dict[tuple[str, str, int], dict[str, str]]:
    rows: dict[tuple[str, str, int], dict[str, str]] = {}
    pattern = f"*_{{suffix}}.csv".format(suffix=suffix)
    for csv_path in sorted(results_dir.glob(pattern)):
        with csv_path.open("r", encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            for row in reader:
                key = (row["algorithm"], row["params"], int(row["batch_size"]))
                rows[key] = row
    return rows


def build_combined_rows(seq_rows: dict, batch_rows: dict, algo_filter: str) -> list[dict[str, float | int | str]]:
    keys = sorted(set(seq_rows) | set(batch_rows))
    combined: list[dict[str, float | int | str]] = []

    for key in keys:
        algorithm, params, batch_size = key
        if algo_filter != "all" and algorithm != algo_filter:
            continue

        seq = seq_rows.get(key)
        batch = batch_rows.get(key)
        time_seq = float(seq["time_seq_ms"]) if seq else 0.0
        time_batch = float(batch["time_batch_ms"]) if batch else 0.0
        serialize_seq = float(seq["serialize_time_ms"]) if seq else 0.0
        serialize_batch = float(batch["serialize_time_ms"]) if batch else 0.0
        peak_memory = float(batch["peak_memory_MB"]) if batch else float(seq["peak_memory_MB"]) if seq else 0.0
        total_sig_size = float(batch["total_sig_size_MB"]) if batch else float(seq["total_sig_size_MB"]) if seq else 0.0
        speedup = time_seq / time_batch if time_seq > 0.0 and time_batch > 0.0 else 0.0

        combined.append(
            {
                "algorithm": algorithm,
                "params": params,
                "batch_size": batch_size,
                "time_seq_ms": time_seq,
                "time_batch_ms": time_batch,
                "speedup": speedup,
                "peak_memory_MB": peak_memory,
                "total_sig_size_MB": total_sig_size,
                "serialize_time_seq_ms": serialize_seq,
                "serialize_time_batch_ms": serialize_batch,
            }
        )

    return combined


def write_raw_csv(rows: list[dict[str, float | int | str]], output_path: Path) -> None:
    fieldnames = [
        "algorithm",
        "params",
        "batch_size",
        "time_seq_ms",
        "time_batch_ms",
        "speedup",
        "peak_memory_MB",
        "total_sig_size_MB",
        "serialize_time_seq_ms",
        "serialize_time_batch_ms",
    ]
    with output_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def render_plots(rows: list[dict[str, float | int | str]], plots_dir: Path) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise RuntimeError("matplotlib is required for plot generation") from exc

    grouped: dict[str, list[dict[str, float | int | str]]] = defaultdict(list)
    for row in rows:
        grouped[str(row["algorithm"])].append(row)

    if not grouped:
        return

    plt.style.use("ggplot")

    fig, ax = plt.subplots(figsize=(9, 5))
    for algorithm, points in sorted(grouped.items()):
        ordered = sorted(points, key=lambda item: int(item["batch_size"]))
        ax.plot(
            [int(item["batch_size"]) for item in ordered],
            [float(item["speedup"]) for item in ordered],
            marker="o",
            linewidth=2,
            label=algorithm,
        )

    ax.set_title("Sequential vs Batch Signing Speedup")
    ax.set_xlabel("Batch size")
    ax.set_ylabel("Speedup = time_seq / time_batch")
    ax.set_xscale("log", base=2)
    ax.set_xticks(sorted({int(row["batch_size"]) for row in rows}))
    ax.get_xaxis().set_major_formatter(plt.ScalarFormatter())
    ax.legend()
    ax.grid(True, which="both", alpha=0.3)
    fig.tight_layout()
    fig.savefig(plots_dir / "speedup_vs_batch.png", dpi=150)
    plt.close(fig)

    for algorithm, points in sorted(grouped.items()):
        ordered = sorted(points, key=lambda item: int(item["batch_size"]))
        fig, ax = plt.subplots(figsize=(9, 5))
        batches = [int(item["batch_size"]) for item in ordered]
        seq_times = [float(item["time_seq_ms"]) for item in ordered]
        batch_times = [float(item["time_batch_ms"]) for item in ordered]

        ax.plot(batches, seq_times, marker="o", linewidth=2, label="sequential")
        ax.plot(batches, batch_times, marker="s", linewidth=2, label="batch")
        ax.set_title(f"{algorithm}: sequential vs batch timings")
        ax.set_xlabel("Batch size")
        ax.set_ylabel("Time, ms")
        ax.set_xscale("log", base=2)
        ax.set_xticks(batches)
        ax.get_xaxis().set_major_formatter(plt.ScalarFormatter())
        ax.grid(True, which="both", alpha=0.3)
        ax.legend()
        fig.tight_layout()
        fig.savefig(plots_dir / f"{algorithm}_times.png", dpi=150)
        plt.close(fig)


def main() -> int:
    args = parse_args()
    results_dir = Path(args.results_dir).resolve()
    output_dir = Path(args.output_dir).resolve() if args.output_dir else results_dir
    plots_dir = output_dir / "plots"

    if not results_dir.is_dir():
        raise SystemExit(f"results directory not found: {results_dir}")

    seq_rows = load_rows(results_dir, "seq")
    batch_rows = load_rows(results_dir, "batch")
    combined_rows = build_combined_rows(seq_rows, batch_rows, args.algo)
    if not combined_rows:
        raise SystemExit("no matching benchmark rows found")

    output_dir.mkdir(parents=True, exist_ok=True)
    plots_dir.mkdir(parents=True, exist_ok=True)
    write_raw_csv(combined_rows, output_dir / "raw_data.csv")

    try:
        render_plots(combined_rows, plots_dir)
    except RuntimeError as exc:
        print(f"[plot] warning: {exc}")

    print(f"[plot] wrote {(output_dir / 'raw_data.csv')} and plots in {plots_dir}")
    if args.copy_to_docs:
        docs_dir = Path(args.docs_dir).resolve()
        docs_dir.mkdir(parents=True, exist_ok=True)
        # copy raw_data.csv
        src_raw = output_dir / "raw_data.csv"
        if src_raw.exists():
            shutil.copy2(src_raw, docs_dir / "raw_data.csv")
        # copy all png plots
        for p in plots_dir.glob("*.png"):
            try:
                shutil.copy2(p, docs_dir / p.name)
            except Exception as e:
                print(f"[plot] warning: failed to copy {p}: {e}")

        print(f"[plot] copied raw_data.csv and plots to {docs_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())