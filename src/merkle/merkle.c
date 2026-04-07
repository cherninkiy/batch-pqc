#include "merkle.h"
#include <stdlib.h>
#include <string.h>

struct merkle_tree {
    size_t num_leaves;
    size_t leaf_capacity;
    size_t hash_size;
    uint8_t* tree;
    merkle_hash_func hash;
};

/* Return next power of two that is >= n. */
static size_t next_power_of_two(size_t n) {
    if (n == 0) {
        return 1;
    }

    size_t p = 1;
    while (p < n) {
        p <<= 1;
    }

    return p;
}

merkle_tree_t* merkle_tree_create(size_t max_leaves, size_t hash_size, merkle_hash_func hash) {
    if (max_leaves == 0 || hash_size == 0 || hash == NULL) {
        return NULL;
    }

    size_t cap = next_power_of_two(max_leaves);
    size_t tree_size = 2 * cap * hash_size;

    uint8_t* tree = (uint8_t*)calloc(1, tree_size);
    if (tree == NULL) {
        return NULL;
    }

    merkle_tree_t* mt = (merkle_tree_t*)malloc(sizeof(merkle_tree_t));
    if (mt == NULL) {
        free(tree);
        return NULL;
    }

    mt->num_leaves = 0;
    mt->leaf_capacity = cap;
    mt->hash_size = hash_size;
    mt->tree = tree;
    mt->hash = hash;

    return mt;
}

void merkle_tree_destroy(merkle_tree_t* mt) {
    if (mt) {
        free(mt->tree);
        free(mt);
    }
}

int merkle_tree_set_leaf(merkle_tree_t* mt, size_t idx, const uint8_t* hash) {
    if (mt == NULL || idx >= mt->leaf_capacity || hash == NULL) {
        return -1;
    }

    size_t pos = mt->leaf_capacity + idx;
    memcpy(mt->tree + pos * mt->hash_size, hash, mt->hash_size);

    if (idx + 1 > mt->num_leaves) {
        mt->num_leaves = idx + 1;
    }

    return 0;
}

int merkle_tree_build(merkle_tree_t* mt) {
    if (mt == NULL || mt->num_leaves == 0) {
        return -1;
    }

    size_t buf_len = 2 * mt->hash_size;
    uint8_t* buf = (uint8_t*)malloc(buf_len);
    if (buf == NULL) {
        return -1;
    }

    /* Build tree bottom-up. Unset leaves remain zero-initialized by calloc. */
    for (size_t i = mt->leaf_capacity - 1; i > 0; --i) {
        uint8_t* left = mt->tree + (2 * i) * mt->hash_size;
        uint8_t* right = mt->tree + (2 * i + 1) * mt->hash_size;
        uint8_t* out = mt->tree + i * mt->hash_size;

        memcpy(buf, left, mt->hash_size);
        memcpy(buf + mt->hash_size, right, mt->hash_size);
        mt->hash(buf, buf_len, out);
    }

    free(buf);
    return 0;
}

const uint8_t* merkle_tree_root(merkle_tree_t* mt) {
    if (mt == NULL || mt->num_leaves == 0) {
        return NULL;
    }

    return mt->tree + mt->hash_size;
}

size_t merkle_tree_leaf_count(merkle_tree_t* mt) {
    return mt ? mt->num_leaves : 0;
}

size_t merkle_tree_capacity(merkle_tree_t* mt) {
    return mt ? mt->leaf_capacity : 0;
}

uint8_t** merkle_tree_get_proof(merkle_tree_t* mt, size_t leaf_idx, size_t* out_len) {
    if (mt == NULL || leaf_idx >= mt->num_leaves || out_len == NULL) {
        return NULL;
    }

    size_t idx = mt->leaf_capacity + leaf_idx;
    size_t proof_capacity = 0;
    size_t proof_size = 0;
    uint8_t** proof = NULL;

    while (idx > 1) {
        size_t sibling = idx ^ 1;

        if (proof_size >= proof_capacity) {
            proof_capacity = proof_capacity ? proof_capacity * 2 : 8;
            uint8_t** new_proof = (uint8_t**)realloc(proof, proof_capacity * sizeof(uint8_t*));
            if (new_proof == NULL) {
                merkle_tree_free_proof(proof, proof_size);
                *out_len = 0;
                return NULL;
            }
            proof = new_proof;
        }

        uint8_t* sibling_hash = (uint8_t*)malloc(mt->hash_size);
        if (sibling_hash == NULL) {
            merkle_tree_free_proof(proof, proof_size);
            *out_len = 0;
            return NULL;
        }

        memcpy(sibling_hash, mt->tree + sibling * mt->hash_size, mt->hash_size);
        proof[proof_size++] = sibling_hash;
        idx >>= 1;
    }

    *out_len = proof_size;
    return proof;
}

void merkle_tree_free_proof(uint8_t** proof, size_t len) {
    if (proof) {
        for (size_t i = 0; i < len; ++i) {
            free(proof[i]);
        }
        free(proof);
    }
}

int merkle_tree_verify_proof(const uint8_t* leaf_hash,
                              uint8_t** proof, size_t proof_len,
                              size_t leaf_idx,
                              const uint8_t* root_hash,
                              size_t hash_size,
                              merkle_hash_func hash) {
    if (leaf_hash == NULL || root_hash == NULL || hash == NULL || hash_size == 0) {
        return 0;
    }
    if (proof_len > 0 && proof == NULL) {
        return 0;
    }

    uint8_t* current = (uint8_t*)malloc(hash_size);
    uint8_t* buf = (uint8_t*)malloc(2 * hash_size);
    if (current == NULL || buf == NULL) {
        free(current);
        free(buf);
        return 0;
    }

    memcpy(current, leaf_hash, hash_size);

    for (size_t i = 0; i < proof_len; ++i) {
        uint8_t* left;
        uint8_t* right;

        if ((leaf_idx >> i) & 1) {
            left = proof[i];
            right = current;
        } else {
            left = current;
            right = proof[i];
        }

        memcpy(buf, left, hash_size);
        memcpy(buf + hash_size, right, hash_size);
        hash(buf, 2 * hash_size, current);
    }

    int result = (memcmp(current, root_hash, hash_size) == 0);
    free(current);
    free(buf);
    return result;
}