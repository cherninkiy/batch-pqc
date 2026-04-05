# third_party

This directory contains git submodules for cryptographic primitives and Merkle tree logic used in batch-pqc.

- shipovnik/          → https://github.com/QAPP-tech/shipovnik_tc26
- hypericum/          → https://github.com/QAPP-tech/hypericum_tc26
- kryzhovnik/         → https://github.com/ElenaKirshanova/pqc_LWR_signature
- iaik_merkle_tree/   → https://github.com/IAIK/merkle-tree

To initialize all submodules:

    git submodule update --init --recursive
