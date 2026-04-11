#ifndef MERKLE_H
#define MERKLE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct merkle_tree merkle_tree_t;

typedef void (*merkle_hash_func)(const uint8_t* in, size_t in_len, uint8_t* out);

/* Create a tree for max_leaves leaves (padded to power-of-two capacity). */
merkle_tree_t* merkle_tree_create(size_t max_leaves, size_t hash_size, merkle_hash_func hash);

/* Destroy a tree and release owned memory. */
void merkle_tree_destroy(merkle_tree_t* tree);

/* Set one leaf hash (copies hash_size bytes). */
int merkle_tree_set_leaf(merkle_tree_t* tree, size_t idx, const uint8_t* hash);

/* Build all internal nodes. Must be called after leaves are set. */
int merkle_tree_build(merkle_tree_t* tree);

/* Return root pointer (hash_size bytes), owned by tree. */
const uint8_t* merkle_tree_root(merkle_tree_t* tree);

/* Return effective number of leaves that were set. */
size_t merkle_tree_leaf_count(merkle_tree_t* tree);

/* Return tree capacity (next power of two). */
size_t merkle_tree_capacity(merkle_tree_t* tree);

/*
 * Get proof path for leaf_idx.
 * Returns an array of sibling-hash pointers and sets *out_len.
 * For a single-leaf tree (*out_len == 0) returns NULL; this is a valid empty
 * proof, not an error. For all larger trees, a NULL return means failure.
 * Caller must release a non-NULL return with merkle_tree_free_proof().
 */
uint8_t** merkle_tree_get_proof(merkle_tree_t* tree, size_t leaf_idx, size_t* out_len);

/* Release proof returned by merkle_tree_get_proof(). */
void merkle_tree_free_proof(uint8_t** proof, size_t len);

/*
 * Verify proof path without tree object.
 * Returns 1 on success, 0 on mismatch or invalid input.
 * For proof_len == 0 (single-leaf tree), proof may be NULL.
 */
int merkle_tree_verify_proof(const uint8_t* leaf_hash,
                              uint8_t** proof, size_t proof_len,
                              size_t leaf_idx,
                              const uint8_t* root_hash,
                              size_t hash_size,
                              merkle_hash_func hash);

#ifdef __cplusplus
}
#endif

#endif // MERKLE_H