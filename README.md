# batch-pqc

**PoC: Batch signing for Russian Post-Quantum Cryptography algorithms**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![CI](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml/badge.svg)](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml)

**[Русская версия](README_ru.md)**

## Overview

This project demonstrates **batch signing** (Merkle tree based) for three Russian post-quantum signature schemes:

- **Shipovnik** (based on Stern protocol)
- **Hypericum** (stateless, based on SPHINCS+, uses **Streebog** hash function from its own implementation)
- **Kryzhovnik** (lattice-based, similar to Dilithium)

The goal is to measure the speedup of signing a batch of messages with a single root signature + proofs, compared to sequential signing. The results will help to assess the feasibility of batch signing in high-load PKI systems.

**This is an open-source PoC for portfolio and partnership purposes. The code is released under MIT license.**

## Features

- Generic Merkle tree implementation (wrapper around [IAIK/merkle-tree](https://github.com/IAIK/merkle-tree))
- Hash abstraction layer (supports **Streebog** via Hypericum implementation, SHA-256 for fallback/testing)
- Signature abstraction layer for:
  - Russian PQC: [Shipovnik](https://github.com/QAPP-tech/shipovnik_tc26), [Hypericum](https://github.com/QAPP-tech/hypericum_tc26), [Kryzhovnik](https://github.com/ElenaKirshanova/pqc_LWR_signature)
  - (planned) ML-DSA (FIPS 204) via liboqs
- Batch signing and verification API
- Microbenchmarks: sequential vs batch signing, verification overhead
- Results exported as CSV + plots

## Repository Structure

```
batch-pqc/
├── third_party/            # git submodules
│   ├── shipovnik/          → https://github.com/QAPP-tech/shipovnik_tc26
│   ├── hypericum/          → https://github.com/QAPP-tech/hypericum_tc26
│   ├── kryzhovnik/         → https://github.com/ElenaKirshanova/pqc_LWR_signature
│   └── iaik_merkle_tree/   → https://github.com/IAIK/merkle-tree
├── src/
│   ├── hash/               # hash provider (uses Streebog from hypericum, SHA-256 fallback)
│   ├── signature/          # signature provider adapters
│   ├── merkle/             # Merkle wrapper over IAIK
│   ├── batch/              # batch signing logic
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

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run benchmarks (sequential vs batch)
./bench/bench_batch --algo hypericum --batch-size 16 --iterations 100
```

### Expected Output (after MVP)

After running benchmarks, the `results/` directory will contain:

- `raw_data.csv` – timings for each (algorithm, batch_size)
- `plots/speedup_vs_batch.png` – graph for all three algorithms

## Current Status (MVP)

- [ ] Repository structure with submodules
- [ ] Hash abstraction + Streebog integration (from hypericum)
- [ ] Merkle tree wrapper (uses IAIK tree)
- [ ] Batch signer/verifier with null signature (test mode)
- [ ] Adapters for real PQC algorithms (in progress: Hypericum first)
- [ ] Full benchmark suite with results
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
- [Elena Kirshanova](https://github.com/ElenaKirshanova) – Kryzhovnik (lattice‑based) signature
