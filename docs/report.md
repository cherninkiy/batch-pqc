# Final Report: Batch PQC Benchmarking

> Note: This report is maintained as Markdown (`docs/report.md`). The repository no longer provides an automated PDF generation path. If you need a PDF version, convert the Markdown locally using `pandoc` and a LaTeX distribution.

## 1. Goal

This project evaluates whether Merkle-tree-based batch signing can reduce total signing cost for Russian post-quantum signature schemes when signing many independent messages. The comparison baseline is straightforward sequential signing of the same batch.

The implementation targets three schemes already integrated in the repository:

- Shipovnik
- Hypericum
- Kryzhovnik

## 2. Architecture

The benchmark stack is split into four layers:

1. `src/signature/*` exposes a unified `bb_algorithm` interface for key generation, signing, and verification.
2. `src/batch_adapters.*` bridges the benchmark-facing `bb_algorithm` interface to the callback-oriented batch signing core.
3. `src/batch_signing.*` implements Merkle-tree-based batch signing and verification.
4. `bench/bench_seq.c` and `bench/bench_batch.c` measure the sequential and real batch workflows with a compatible CSV/JSON format.

In the batch path, each message becomes a Merkle leaf after hashing with Streebog-256. The root hash is signed once, and each message receives its authentication proof. Verification reconstructs the root from the message hash and proof, then checks the single root signature.

## 3. Measurement Methodology

Both benchmark executables support the same core parameters:

- `--algo`
- `--batch-size`
- `--iters`
- `--warmup`
- `--msg-size`
- `--seed`
- `--verify`

Measured metrics:

- `keygen_ms`: one-time key generation cost per benchmark run
- `time_seq_ms`: total time for sequential signing across measured iterations
- `time_batch_ms`: total time for real batch signing across measured iterations
- `verify_ms`: total verification time when `--verify 1`
- `serialize_time_ms`: time spent serializing produced signatures
- `peak_memory_MB`: process peak RSS during the run
- `total_sig_size_MB`: average serialized signature payload per measured iteration

Sequential runs and batch runs are emitted separately as `*_seq.csv` and `*_batch.csv`. The plotting step joins both datasets by `(algorithm, params, batch_size)` and computes:

$$
speedup = \frac{time\_seq\_ms}{time\_batch\_ms}
$$

## 4. Running the Full Comparison

Example workflow:

```bash
./scripts/benchmark.sh --algo all --batch-sizes 1,2,4,8,16,32,64 --iters 100 --verify 1
```

Generated artifacts:

- `results/bench-<timestamp>/*_seq.csv`
- `results/bench-<timestamp>/*_batch.csv`
- `results/bench-<timestamp>/raw_data.csv`
- `results/bench-<timestamp>/plots/speedup_vs_batch.png`
- `results/bench-<timestamp>/plots/<algorithm>_times.png`

The helper script invokes `scripts/plot_results.py` automatically when `python3` is available. Plot generation additionally requires `matplotlib`.

## 5. Interpreting the Results

The main chart is `speedup_vs_batch.png`.

- `speedup > 1` means the Merkle-based batch path outperforms sequential signing for that batch size.
- `speedup = 1` means both strategies are equivalent.
- `speedup < 1` means the Merkle tree overhead is still larger than the saved signing work.

The per-algorithm timing plots are useful to separate two effects:

- whether root-signature amortization dominates total cost
- whether proof construction and serialization become the bottleneck for larger batches

### Measured Results (run: `results/bench-20260423-004124`)

Observed speedup ranges on this machine:

![Speedup](figures/speedup_vs_batch.png)

**Per-algorithm timings:**


| Shipovnik | Hypericum | Kryzhovnik |
| :-: | :-: | :-: |
| ![Shipovnik](figures/shipovnik_times.png) | ![Hypericum](figures/hypericum_times.png) | ![Kryzhovnik](figures/kryzhovnik_times.png) |


- Shipovnik: from `1.01x` at `batch_size=1` up to `62.34x` at `batch_size=64`.
- Hypericum (`debug`): from `0.99x` at `batch_size=1` up to `63.23x` at `batch_size=64`.
- Kryzhovnik (`debug`): from `0.15x` to `0.44x` (batch mode slower than sequential for all tested sizes).

Interpretation:

- For Shipovnik and Hypericum, batch signing shows near-linear amortization as batch size grows.
- For Kryzhovnik in the current implementation and parameter set, Merkle overhead dominates and no crossover point was observed up to `batch_size=64`.
- Signature payload behavior matches the design expectation: batch mode keeps approximately one root signature plus proofs, while sequential mode grows linearly with the number of signed messages.

## 6. Practical Notes

- Hypericum and Kryzhovnik paramsets are build-time selectable through CMake.
- The benchmark uses deterministic message generation for reproducibility.
- `debug` paramsets remain suitable only for diagnostics and quick iteration, not for security claims.
- This repository is still a PoC and is not intended for production deployment.

## 7. Conclusion

The repository now contains the full comparison pipeline needed for the MVP:

- sequential benchmark
- real batch benchmark
- common automation script
- aggregated CSV output
- plot generation
- reproducible reportable workflow

For this benchmark run, crossover behavior is already clear:

- Shipovnik: batch is beneficial from `batch_size >= 1` and scales strongly.
- Hypericum (`debug`): batch becomes beneficial from `batch_size >= 2`.
- Kryzhovnik (`debug`): batch is not beneficial in the tested range and requires further optimization or different parameter choices.