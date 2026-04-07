# third_party

This directory contains git submodules for cryptographic primitives and Merkle tree logic used in batch-pqc.

Original algorithm repositories:

- Shipovnik: https://github.com/QAPP-tech/shipovnik_tc26
- Hypericum: https://github.com/QAPP-tech/hypericum_tc26
- Kryzhovnik: https://github.com/ElenaKirshanova/pqc_LWR_signature

Current submodule remotes used by this project:

- shipovnik/          → https://github.com/cherninkiy/shipovnik-wrapper-tc26
- hypericum/          → https://github.com/cherninkiy/hypericum-wrapper-tc26
- kryzhovnik/         → https://github.com/cherninkiy/kryzhovnik-wrapper-tc26
- iaik_merkle_tree/   → https://github.com/IAIK/merkle-tree

Fork rationale:

- Add integration-oriented APIs (detached/status-return wrappers)
- Keep adapter contracts consistent across all algorithms
- Preserve reproducible pinned revisions for CI and local builds

To initialize all submodules:

    git submodule sync --recursive
    git submodule update --init --recursive

Build logic for shipovnik, hypericum, streebog, and kryzhovnik is centralized in third_party/CMakeLists.txt.
The project does not rely on the CMakeLists.txt files shipped inside those submodules.
