# third_party

This directory contains git submodules for cryptographic primitives and Merkle tree logic used in batch-pqc.

- shipovnik/          → https://github.com/QAPP-tech/shipovnik_tc26
- hypericum/          → https://github.com/QAPP-tech/hypericum_tc26
- kryzhovnik/         → https://github.com/cherninkiy/kryzhovnik-wrapper-tc26
- iaik_merkle_tree/   → https://github.com/IAIK/merkle-tree

The original Kryzhovnik repository is https://github.com/ElenaKirshanova/pqc_LWR_signature.
This project uses a compatibility fork for the `third_party/kryzhovnik` submodule to keep compatibility with hypericum and shipovnik.

To initialize all submodules:

    git submodule sync --recursive
    git submodule update --init --recursive

Build logic for shipovnik, hypericum, streebog, and kryzhovnik is centralized in third_party/CMakeLists.txt.
The project does not rely on the CMakeLists.txt files shipped inside those submodules.
