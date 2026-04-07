# batch-pqc

**PoC: Batch signing for Russian Post-Quantum Cryptography algorithms**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![CI](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml/badge.svg)](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml)

**[Русская версия](README_ru.md)**

## Overview

This project demonstrates **batch signing** (Merkle tree based) for three Russian post-quantum signature schemes:

- [Shipovnik](https://github.com/QAPP-tech/shipovnik_tc26) (based on Stern protocol)
- [Hypericum](https://github.com/QAPP-tech/hypericum_tc26) (stateless, based on SPHINCS+, uses **Streebog** hash function from its own implementation)
- [Kryzhovnik](https://github.com/ElenaKirshanova/pqc_LWR_signature) (lattice-based, similar to Dilithium)

The goal is to measure the speedup of signing a batch of messages with a single root signature + proofs, compared to sequential signing. The results will help to assess the feasibility of batch signing in high-load PKI systems.

**This is an open-source PoC for portfolio and partnership purposes. The code is released under MIT license.**

## Features

- (planned) Generic Merkle tree implementation (wrapper around [IAIK/merkle-tree](https://github.com/IAIK/merkle-tree))
- (planned) Hash abstraction layer (supports **Streebog** via Hypericum implementation, SHA-256 for fallback/testing)
- Signature abstraction layer for Russian PQC:
  - [Shipovnik](https://github.com/QAPP-tech/shipovnik_tc26)
  - [Hypericum](https://github.com/QAPP-tech/hypericum_tc26)
  - [Kryzhovnik](https://github.com/ElenaKirshanova/pqc_LWR_signature)
- Sequential benchmark (`bench_seq`) with configurable paramsets, verification pass, and warmup iterations
- Test suite with unified `test_*` naming in CTest

## Original Repos and Forks

Algorithm links in this README always point to original repositories:

- Shipovnik (original): https://github.com/QAPP-tech/shipovnik_tc26
- Hypericum (original): https://github.com/QAPP-tech/hypericum_tc26
- Kryzhovnik (original): https://github.com/ElenaKirshanova/pqc_LWR_signature

This project currently integrates wrapper/compatibility forks as submodules:

- https://github.com/cherninkiy/shipovnik-wrapper-tc26
- https://github.com/cherninkiy/hypericum-wrapper-tc26
- https://github.com/cherninkiy/kryzhovnik-wrapper-tc26

Why forks are used:

- To add integration-oriented APIs required by this project (detached and status-return wrappers)
- To keep adapter contracts consistent across all three algorithms
- To preserve reproducible pinned revisions for CI and local builds

## Repository Structure

```
batch-pqc/
├── third_party/            # git submodules
│   ├── shipovnik/          → https://github.com/cherninkiy/shipovnik-wrapper-tc26
│   ├── hypericum/          → https://github.com/cherninkiy/hypericum-wrapper-tc26
│   ├── kryzhovnik/         → https://github.com/cherninkiy/kryzhovnik-wrapper-tc26
│   └── iaik_merkle_tree/   → https://github.com/IAIK/merkle-tree
├── src/
│   ├── signature/          # signature provider adapters
│   └── utils/              # timers, message generators
├── bench/                  # benchmarking executables
├── tests/                  # unit tests (CTest)
├── scripts/                # build & benchmark automation
├── results/                # CSV and plots (auto-generated)
└── docs/                   # architecture and final report
```

## Build & Run (MVP)

### Prerequisites

- Linux x86_64 (tested on Ubuntu 22.04)
- CMake 3.14+, GCC/Clang with C11 support
- OpenSSL (for SHA-256 fallback, optional)

### Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/cherninkiy/batch-pqc.git
cd batch-pqc

# Sync submodule URLs in case .gitmodules changed
git submodule sync --recursive
git submodule update --init --recursive

# Build third-party dependencies and tests
./scripts/third_party.sh build

# Run tests
./scripts/third_party.sh tests

# Run benchmark (sequential signing)
./build/bench/bench_seq --algo hypericum --batch-size 16 --iters 100 --verify 1
```

### Paramset Selection

Both Hypericum and Kryzhovnik support build-time paramset selection via CMake.

```bash
cmake -S . -B build \
  -DHYPERICUM_PARAMSET=m_128_20 \
  -DKRYZHOVNIK_PARAMSET=medium
cmake --build build --parallel
```

Supported Hypericum values: `b_256_64`, `m_256_64`, `b_256_20`, `m_256_20`, `b_128_20`, `m_128_20`, `debug`.

`debug` is an aggressive speed-oriented research profile for benchmarks and diagnostics.

Supported Kryzhovnik values: `small`, `medium`, `large`, `debug`.

`debug` is intended for fast diagnostics and troubleshooting, not security evaluation.

The benchmark helper script also supports separate flags:

```bash
./scripts/benchmark.sh \
  --algo kryzhovnik \
  --kryzhovnik-params small \
  --hypericum-params debug
```

Note: Kryzhovnik constants are aligned with `security.sage` for `small/medium/large`. Keep validating against the official specification before production use.

### Expected Output (after MVP)

After running benchmarks, the `results/` directory will contain:

- `raw_data.csv` – timings for each (algorithm, batch_size)
- `plots/speedup_vs_batch.png` – graph for all three algorithms

## Current Status (MVP)

- [x] Repository structure with submodules
- [x] Adapters for Shipovnik / Hypericum / Kryzhovnik with unified `bb_status`
- [x] Detached/status-return integration APIs wired through wrapper submodules
- [x] Sequential benchmark `bench_seq` with warmup and corrected signature-size metric
- [x] Test coverage: `test_adapters_smoke`, `test_adapters_batch`, `test_kryzhovnik*`, `test_merkletree`
- [ ] Merkle-based batch signer/verifier implementation
- [ ] Sequential vs real batch signing benchmark comparison
- [ ] Final report (PDF)

**MVP is being developed in `dev` branch. After completion, a Pull Request to `main` will be opened for review.**

## Contributing and Review

We welcome technical review from:

- **Elena Kirshanova** (Kryzhovnik author)
- **QAPP team** (Shipovnik, Hypericum authors)

If you are one of the authors, please check the [Pull Request](https://github.com/cherninkiy/batch-pqc/pull/1) (once opened) and leave your comments. For general feedback, use GitHub Issues.

## License

This project is licensed under the **MIT License** – see [LICENSE](LICENSE) file for details.

Third-party components have their own licenses:

- `iaik_merkle_tree` – public domain / BSD-like
- PQC implementations – please refer to each submodule's license

## Acknowledgments

- [IAIK/merkle-tree](https://github.com/IAIK/merkle-tree) – Merkle tree implementation (public domain / BSD‑like)
- [QAPP-tech](https://github.com/QAPP-tech) – Shipovnik and Hypericum (including Streebog hash) PQC signatures
- [Elena Kirshanova](https://github.com/ElenaKirshanova) – original Kryzhovnik implementation: [pqc_LWR_signature](https://github.com/ElenaKirshanova/pqc_LWR_signature)
- A compatibility fork is used in this project to integrate Kryzhovnik with Hypericum and Shipovnik

---

**Warning:** This is an open-source PoC for portfolio and partnership purposes. The code is released under the MIT license. It is not intended for production use. Use at your own risk.
