#include "batch_signing.h"
#include "merkle/merkle.h"

#include <stdlib.h>
#include <string.h>

#define BATCH_SIGNATURE_VERSION 1u

struct batch_signer {
    size_t hash_size;
    size_t root_sig_capacity;
    batch_hash_fn hash;
    batch_sign_fn sign;
    batch_verify_fn verify;
    void* user_ctx;

    uint8_t** messages;
    size_t* msg_lens;
    size_t num_msgs;
    size_t capacity;
};

struct batch_signature {
    size_t hash_size;

    uint8_t* root_hash;
    uint8_t* root_signature;
    size_t root_sig_len;

    uint8_t*** proofs;
    size_t* proof_lens;
    size_t num_proofs;
};

static int write_u32(uint8_t** ptr, const uint8_t* end, uint32_t value) {
    if ((size_t)(end - *ptr) < sizeof(uint32_t)) {
        return -1;
    }
    memcpy(*ptr, &value, sizeof(uint32_t));
    *ptr += sizeof(uint32_t);
    return 0;
}

static int read_u32(const uint8_t** ptr, const uint8_t* end, uint32_t* out) {
    if ((size_t)(end - *ptr) < sizeof(uint32_t)) {
        return -1;
    }
    memcpy(out, *ptr, sizeof(uint32_t));
    *ptr += sizeof(uint32_t);
    return 0;
}

static int batch_signer_grow(batch_signer_t* ctx, size_t new_capacity) {
    if (new_capacity <= ctx->capacity) {
        return 0;
    }

    size_t cap = ctx->capacity ? ctx->capacity * 2 : 16;
    while (cap < new_capacity) {
        cap *= 2;
    }

    uint8_t** new_msgs = (uint8_t**)realloc(ctx->messages, cap * sizeof(uint8_t*));
    if (new_msgs == NULL) {
        return -1;
    }

    size_t* new_lens = (size_t*)realloc(ctx->msg_lens, cap * sizeof(size_t));
    if (new_lens == NULL) {
        return -1;
    }

    ctx->messages = new_msgs;
    ctx->msg_lens = new_lens;
    ctx->capacity = cap;
    return 0;
}

batch_signer_t* batch_signer_create(size_t hash_size,
                                    size_t root_sig_capacity,
                                    batch_hash_fn hash,
                                    batch_sign_fn sign,
                                    batch_verify_fn verify,
                                    void* user_ctx) {
    if (hash_size == 0 || root_sig_capacity == 0 || hash == NULL || sign == NULL) {
        return NULL;
    }

    batch_signer_t* ctx = (batch_signer_t*)calloc(1, sizeof(batch_signer_t));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->hash_size = hash_size;
    ctx->root_sig_capacity = root_sig_capacity;
    ctx->hash = hash;
    ctx->sign = sign;
    ctx->verify = verify;
    ctx->user_ctx = user_ctx;
    ctx->capacity = 16;

    ctx->messages = (uint8_t**)malloc(ctx->capacity * sizeof(uint8_t*));
    ctx->msg_lens = (size_t*)malloc(ctx->capacity * sizeof(size_t));

    if (ctx->messages == NULL || ctx->msg_lens == NULL) {
        free(ctx->messages);
        free(ctx->msg_lens);
        free(ctx);
        return NULL;
    }

    memset(ctx->messages, 0, ctx->capacity * sizeof(uint8_t*));
    memset(ctx->msg_lens, 0, ctx->capacity * sizeof(size_t));

    return ctx;
}

void batch_signer_free(batch_signer_t* ctx) {
    if (ctx == NULL) {
        return;
    }

    batch_signer_reset(ctx);
    free(ctx->messages);
    free(ctx->msg_lens);
    free(ctx);
}

int batch_signer_add_message(batch_signer_t* ctx, const uint8_t* msg, size_t msg_len) {
    if (ctx == NULL || msg == NULL) {
        return -1;
    }

    if (ctx->num_msgs >= ctx->capacity) {
        if (batch_signer_grow(ctx, ctx->num_msgs + 1) != 0) {
            return -1;
        }
    }

    uint8_t* copy = (uint8_t*)malloc(msg_len);
    if (copy == NULL) {
        return -1;
    }

    memcpy(copy, msg, msg_len);
    ctx->messages[ctx->num_msgs] = copy;
    ctx->msg_lens[ctx->num_msgs] = msg_len;
    ctx->num_msgs++;

    return 0;
}

void batch_signer_reset(batch_signer_t* ctx) {
    if (ctx == NULL) {
        return;
    }

    for (size_t i = 0; i < ctx->num_msgs; ++i) {
        free(ctx->messages[i]);
        ctx->messages[i] = NULL;
    }
    ctx->num_msgs = 0;
}

size_t batch_signer_count(const batch_signer_t* ctx) {
    if (ctx == NULL) {
        return 0;
    }
    return ctx->num_msgs;
}

batch_signature_t* batch_signer_sign(batch_signer_t* ctx) {
    if (ctx == NULL || ctx->num_msgs == 0) {
        return NULL;
    }

    uint8_t** leaf_hashes = (uint8_t**)malloc(ctx->num_msgs * sizeof(uint8_t*));
    if (leaf_hashes == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < ctx->num_msgs; ++i) {
        leaf_hashes[i] = (uint8_t*)malloc(ctx->hash_size);
        if (leaf_hashes[i] == NULL) {
            for (size_t j = 0; j < i; ++j) free(leaf_hashes[j]);
            free(leaf_hashes);
            return NULL;
        }
        ctx->hash(ctx->messages[i], ctx->msg_lens[i], leaf_hashes[i]);
    }

    merkle_tree_t* tree = merkle_tree_create(ctx->num_msgs, ctx->hash_size, ctx->hash);
    if (tree == NULL) {
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        return NULL;
    }
    
    for (size_t i = 0; i < ctx->num_msgs; ++i) {
            if (merkle_tree_set_leaf(tree, i, leaf_hashes[i]) != 0) {
                merkle_tree_destroy(tree);
                for (size_t j = 0; j < ctx->num_msgs; ++j) free(leaf_hashes[j]);
                free(leaf_hashes);
                return NULL;
            }
    }
        if (merkle_tree_build(tree) != 0) {
            merkle_tree_destroy(tree);
            for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
            free(leaf_hashes);
            return NULL;
        }

    const uint8_t* root_hash = merkle_tree_root(tree);
    if (root_hash == NULL) {
        merkle_tree_destroy(tree);
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        return NULL;
    }

    size_t root_sig_len = 0;
    uint8_t* root_sig = (uint8_t*)malloc(ctx->root_sig_capacity);
    if (root_sig == NULL) {
        merkle_tree_destroy(tree);
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        return NULL;
    }

    if (!ctx->sign(ctx->user_ctx,
                   root_hash,
                   ctx->hash_size,
                   root_sig,
                   ctx->root_sig_capacity,
                   &root_sig_len) ||
        root_sig_len == 0 ||
        root_sig_len > ctx->root_sig_capacity) {
        free(root_sig);
        merkle_tree_destroy(tree);
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        return NULL;
    }

    size_t num_proofs = ctx->num_msgs;
    uint8_t*** proofs = (uint8_t***)malloc(num_proofs * sizeof(uint8_t**));
    size_t* proof_lens = (size_t*)malloc(num_proofs * sizeof(size_t));

    if (proofs == NULL || proof_lens == NULL) {
        free(root_sig);
        merkle_tree_destroy(tree);
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        free(proofs);
        free(proof_lens);
        return NULL;
    }

    for (size_t i = 0; i < num_proofs; ++i) {
        proofs[i] = merkle_tree_get_proof(tree, i, &proof_lens[i]);
        if (proof_lens[i] > 0 && proofs[i] == NULL) {
            for (size_t j = 0; j < i; ++j) merkle_tree_free_proof(proofs[j], proof_lens[j]);
            free(proofs);
            free(proof_lens);
            free(root_sig);
            merkle_tree_destroy(tree);
            for (size_t j = 0; j < ctx->num_msgs; ++j) free(leaf_hashes[j]);
            free(leaf_hashes);
            return NULL;
        }
    }

    batch_signature_t* sig = (batch_signature_t*)malloc(sizeof(batch_signature_t));
    if (sig == NULL) {
        for (size_t i = 0; i < num_proofs; ++i) merkle_tree_free_proof(proofs[i], proof_lens[i]);
        free(proofs);
        free(proof_lens);
        free(root_sig);
        merkle_tree_destroy(tree);
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        return NULL;
    }

    sig->hash_size = ctx->hash_size;
    sig->root_signature = root_sig;
    sig->root_sig_len = root_sig_len;
    sig->proofs = proofs;
    sig->proof_lens = proof_lens;
    sig->num_proofs = num_proofs;

    sig->root_hash = (uint8_t*)malloc(ctx->hash_size);
    if (sig->root_hash == NULL) {
        for (size_t i = 0; i < num_proofs; ++i) merkle_tree_free_proof(proofs[i], proof_lens[i]);
        free(proofs);
        free(proof_lens);
        free(root_sig);
        free(sig);
        merkle_tree_destroy(tree);
        for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
        free(leaf_hashes);
        return NULL;
    }
    memcpy(sig->root_hash, root_hash, ctx->hash_size);

    merkle_tree_destroy(tree);
    for (size_t i = 0; i < ctx->num_msgs; ++i) free(leaf_hashes[i]);
    free(leaf_hashes);

    return sig;
}

int batch_signature_serialize(const batch_signature_t* sig, uint8_t* out, size_t* out_len) {
    if (sig == NULL || out_len == NULL || sig->root_hash == NULL || sig->root_signature == NULL) {
        return -1;
    }

    size_t needed = 0;
    needed += sizeof(uint32_t) * 5;
    needed += sig->hash_size;
    needed += sig->root_sig_len;
    for (size_t i = 0; i < sig->num_proofs; ++i) {
        needed += sizeof(uint32_t) + sig->proof_lens[i] * sig->hash_size;
    }

    if (out == NULL) {
        *out_len = needed;
        return 0;
    }

    if (*out_len < needed) {
        return -1;
    }

    uint8_t* ptr = out;
    const uint8_t* end = out + needed;

    if (write_u32(&ptr, end, BATCH_SIGNATURE_VERSION) != 0 ||
        write_u32(&ptr, end, (uint32_t)sig->hash_size) != 0 ||
        write_u32(&ptr, end, (uint32_t)sig->num_proofs) != 0 ||
        write_u32(&ptr, end, (uint32_t)sig->hash_size) != 0) {
        return -1;
    }

    memcpy(ptr, sig->root_hash, sig->hash_size);
    ptr += sig->hash_size;

    if (write_u32(&ptr, end, (uint32_t)sig->root_sig_len) != 0) {
        return -1;
    }
    memcpy(ptr, sig->root_signature, sig->root_sig_len);
    ptr += sig->root_sig_len;

    for (size_t i = 0; i < sig->num_proofs; ++i) {
        if (write_u32(&ptr, end, (uint32_t)sig->proof_lens[i]) != 0) {
            return -1;
        }
        for (size_t j = 0; j < sig->proof_lens[i]; ++j) {
            memcpy(ptr, sig->proofs[i][j], sig->hash_size);
            ptr += sig->hash_size;
        }
    }

    *out_len = needed;
    return 0;
}

batch_signature_t* batch_signature_deserialize(const uint8_t* data, size_t data_len,
                                               size_t hash_size) {
    if (data == NULL || hash_size == 0) {
        return NULL;
    }

    const uint8_t* ptr = data;
    const uint8_t* end = data + data_len;
    uint32_t version = 0;
    uint32_t read_hash_size = 0;
    uint32_t num_proofs = 0;
    uint32_t root_hash_len = 0;
    uint32_t root_sig_len = 0;

    if (read_u32(&ptr, end, &version) != 0 ||
        read_u32(&ptr, end, &read_hash_size) != 0 ||
        read_u32(&ptr, end, &num_proofs) != 0 ||
        read_u32(&ptr, end, &root_hash_len) != 0) {
        return NULL;
    }
    if (version != BATCH_SIGNATURE_VERSION ||
        read_hash_size != (uint32_t)hash_size ||
        root_hash_len != (uint32_t)hash_size ||
        (size_t)(end - ptr) < hash_size) {
        return NULL;
    }

    batch_signature_t* sig = (batch_signature_t*)malloc(sizeof(batch_signature_t));
    if (sig == NULL) {
        return NULL;
    }

    sig->hash_size = hash_size;
    sig->num_proofs = num_proofs;

    sig->root_hash = (uint8_t*)malloc(hash_size);
    if (sig->root_hash == NULL) {
        free(sig);
        return NULL;
    }

    memcpy(sig->root_hash, ptr, hash_size);
    ptr += hash_size;

    if (read_u32(&ptr, end, &root_sig_len) != 0 || (size_t)(end - ptr) < root_sig_len) {
        free(sig->root_hash);
        free(sig);
        return NULL;
    }

    sig->root_sig_len = root_sig_len;
    sig->root_signature = (uint8_t*)malloc(root_sig_len);
    if (sig->root_signature == NULL) {
        free(sig->root_hash);
        free(sig);
        return NULL;
    }
    memcpy(sig->root_signature, ptr, root_sig_len);
    ptr += root_sig_len;

    sig->proofs = (uint8_t***)malloc(num_proofs * sizeof(uint8_t**));
    sig->proof_lens = (size_t*)malloc(num_proofs * sizeof(size_t));

    if (sig->proofs == NULL || sig->proof_lens == NULL) {
        free(sig->root_hash);
        free(sig->root_signature);
        free(sig->proofs);
        free(sig->proof_lens);
        free(sig);
        return NULL;
    }

    for (uint32_t i = 0; i < num_proofs; ++i) {
        uint32_t proof_len = 0;
        if (read_u32(&ptr, end, &proof_len) != 0) {
            for (uint32_t j = 0; j < i; ++j) merkle_tree_free_proof(sig->proofs[j], sig->proof_lens[j]);
            free(sig->proofs);
            free(sig->proof_lens);
            free(sig->root_hash);
            free(sig->root_signature);
            free(sig);
            return NULL;
        }

        sig->proof_lens[i] = proof_len;

        if ((size_t)(end - ptr) < (size_t)proof_len * hash_size) {
            for (uint32_t j = 0; j < i; ++j) merkle_tree_free_proof(sig->proofs[j], sig->proof_lens[j]);
            free(sig->proofs);
            free(sig->proof_lens);
            free(sig->root_hash);
            free(sig->root_signature);
            free(sig);
            return NULL;
        }

        sig->proofs[i] = (uint8_t**)malloc(proof_len * sizeof(uint8_t*));
        if (proof_len > 0 && sig->proofs[i] == NULL) {
            for (uint32_t j = 0; j < i; ++j) merkle_tree_free_proof(sig->proofs[j], sig->proof_lens[j]);
            free(sig->proofs);
            free(sig->proof_lens);
            free(sig->root_hash);
            free(sig->root_signature);
            free(sig);
            return NULL;
        }

        for (uint32_t j = 0; j < proof_len; ++j) {
            sig->proofs[i][j] = (uint8_t*)malloc(hash_size);
            if (sig->proofs[i][j] == NULL) {
                for (uint32_t k = 0; k < j; ++k) free(sig->proofs[i][k]);
                free(sig->proofs[i]);
                for (uint32_t k = 0; k < i; ++k) merkle_tree_free_proof(sig->proofs[k], sig->proof_lens[k]);
                free(sig->proofs);
                free(sig->proof_lens);
                free(sig->root_hash);
                free(sig->root_signature);
                free(sig);
                return NULL;
            }
            memcpy(sig->proofs[i][j], ptr, hash_size);
            ptr += hash_size;
        }
    }

    if (ptr != end) {
        batch_signature_free(sig);
        return NULL;
    }

    return sig;
}

void batch_signature_free(batch_signature_t* sig) {
    if (sig == NULL) {
        return;
    }

    free(sig->root_hash);
    free(sig->root_signature);

    for (size_t i = 0; i < sig->num_proofs; ++i) {
        merkle_tree_free_proof(sig->proofs[i], sig->proof_lens[i]);
    }
    free(sig->proofs);
    free(sig->proof_lens);
    free(sig);
}

int batch_verify(const uint8_t* const* messages,
                 const size_t* msg_lens,
                 size_t num_msgs,
                 const batch_signature_t* sig,
                 size_t hash_size,
                 batch_hash_fn hash,
                 batch_verify_fn verify,
                 void* user_ctx) {
    if (messages == NULL || msg_lens == NULL || sig == NULL || hash == NULL || verify == NULL) {
        return 0;
    }

    if (sig->root_hash == NULL || sig->root_signature == NULL) {
        return 0;
    }
    if (num_msgs != sig->num_proofs || hash_size != sig->hash_size) {
        return 0;
    }

    for (size_t i = 0; i < num_msgs; ++i) {
        uint8_t* leaf_hash = (uint8_t*)malloc(hash_size);
        if (messages[i] == NULL || leaf_hash == NULL) {
            free(leaf_hash);
            return 0;
        }

        hash(messages[i], msg_lens[i], leaf_hash);

        if (!merkle_tree_verify_proof(leaf_hash,
                                      sig->proofs[i],
                                      sig->proof_lens[i],
                                      i,
                                      sig->root_hash,
                                      hash_size,
                                      hash)) {
            free(leaf_hash);
            return 0;
        }
        free(leaf_hash);
    }

    if (!verify(user_ctx,
                sig->root_hash,
                hash_size,
                sig->root_signature,
                sig->root_sig_len)) {
        return 0;
    }

    return 1;
}

const uint8_t* batch_signature_root_hash(const batch_signature_t* sig) {
    if (sig == NULL) {
        return NULL;
    }
    return sig->root_hash;
}

const uint8_t* batch_signature_root_signature(const batch_signature_t* sig, size_t* out_len) {
    if (sig == NULL || out_len == NULL) {
        return NULL;
    }
    *out_len = sig->root_sig_len;
    return sig->root_signature;
}

size_t batch_signature_num_proofs(const batch_signature_t* sig) {
    if (sig == NULL) {
        return 0;
    }
    return sig->num_proofs;
}