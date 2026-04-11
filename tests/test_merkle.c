#include "merkle/merkle.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_hash32(const uint8_t* in, size_t in_len, uint8_t* out) {
    size_t i;

    memset(out, 0, 32);
    for (i = 0; i < in_len; ++i) {
        out[i % 32] ^= (uint8_t)(in[i] + (uint8_t)(i * 13u));
    }
    for (i = 0; i < 32; ++i) {
        out[i] ^= (uint8_t)(0x55u + i);
    }
}

static int test_single_leaf_proof(void) {
    uint8_t leaf[32] = {0};
    const uint8_t* root;
    size_t proof_len = 0;
    uint8_t** proof = NULL;
    merkle_tree_t* tree = merkle_tree_create(1, 32, test_hash32);

    if (tree == NULL) {
        return 1;
    }

    leaf[0] = 0x11u;
    if (merkle_tree_set_leaf(tree, 0, leaf) != 0 || merkle_tree_build(tree) != 0) {
        merkle_tree_destroy(tree);
        return 1;
    }

    root = merkle_tree_root(tree);
    if (root == NULL) {
        merkle_tree_destroy(tree);
        return 1;
    }

    proof = merkle_tree_get_proof(tree, 0, &proof_len);
    if (proof_len != 0) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    if (!merkle_tree_verify_proof(leaf, proof, proof_len, 0, root, 32, test_hash32)) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    merkle_tree_destroy(tree);
    return 0;
}

static int test_even_leaf_proof_and_tamper(void) {
    merkle_tree_t* tree = merkle_tree_create(2, 32, test_hash32);
    uint8_t leaf0[32] = {0};
    uint8_t leaf1[32] = {0};
    size_t proof_len = 0;
    uint8_t** proof = NULL;
    const uint8_t* root;

    if (tree == NULL) {
        return 1;
    }

    leaf0[0] = 0x01u;
    leaf1[0] = 0x02u;
    if (merkle_tree_set_leaf(tree, 0, leaf0) != 0 ||
        merkle_tree_set_leaf(tree, 1, leaf1) != 0 ||
        merkle_tree_build(tree) != 0) {
        merkle_tree_destroy(tree);
        return 1;
    }

    root = merkle_tree_root(tree);
    proof = merkle_tree_get_proof(tree, 0, &proof_len);
    if (proof == NULL || proof_len != 1) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    if (!merkle_tree_verify_proof(leaf0, proof, proof_len, 0, root, 32, test_hash32)) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    proof[0][0] ^= 0x80u;
    if (merkle_tree_verify_proof(leaf0, proof, proof_len, 0, root, 32, test_hash32)) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    merkle_tree_free_proof(proof, proof_len);
    merkle_tree_destroy(tree);
    return 0;
}

static int test_odd_leaf_padding(void) {
    merkle_tree_t* tree = merkle_tree_create(3, 32, test_hash32);
    uint8_t leaf0[32] = {0};
    uint8_t leaf1[32] = {0};
    uint8_t leaf2[32] = {0};
    size_t proof_len = 0;
    uint8_t** proof = NULL;
    const uint8_t* root;

    if (tree == NULL) {
        return 1;
    }

    leaf0[0] = 0x21u;
    leaf1[0] = 0x22u;
    leaf2[0] = 0x23u;

    if (merkle_tree_set_leaf(tree, 0, leaf0) != 0 ||
        merkle_tree_set_leaf(tree, 1, leaf1) != 0 ||
        merkle_tree_set_leaf(tree, 2, leaf2) != 0 ||
        merkle_tree_build(tree) != 0) {
        merkle_tree_destroy(tree);
        return 1;
    }

    if (merkle_tree_capacity(tree) != 4u || merkle_tree_leaf_count(tree) != 3u) {
        merkle_tree_destroy(tree);
        return 1;
    }

    root = merkle_tree_root(tree);
    proof = merkle_tree_get_proof(tree, 1u, &proof_len);
    if (proof == NULL || proof_len != 2u) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    if (!merkle_tree_verify_proof(leaf1, proof, proof_len, 1u, root, 32, test_hash32)) {
        merkle_tree_free_proof(proof, proof_len);
        merkle_tree_destroy(tree);
        return 1;
    }

    merkle_tree_free_proof(proof, proof_len);
    merkle_tree_destroy(tree);
    return 0;
}

int main(void) {
    if (test_single_leaf_proof() != 0) {
        return 1;
    }
    if (test_even_leaf_proof_and_tamper() != 0) {
        return 1;
    }
    if (test_odd_leaf_padding() != 0) {
        return 1;
    }

    printf("test_merkle: ok\n");
    return 0;
}