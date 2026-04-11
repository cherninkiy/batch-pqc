#ifndef BATCH_SIGNING_H
#define BATCH_SIGNING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct batch_signer batch_signer_t;
typedef struct batch_signature batch_signature_t;

/* Hash callback: out = hash(in, in_len), out length is hash_size bytes. */
typedef void (*batch_hash_fn)(const uint8_t* in, size_t in_len, uint8_t* out);

/*
 * Signature callback.
 * Returns 1 on success, 0 on failure.
 */
typedef int (*batch_sign_fn)(void* user_ctx,
                             const uint8_t* msg,
                             size_t msg_len,
                             uint8_t* sig,
                             size_t sig_capacity,
                             size_t* sig_len);

/* Verification callback: returns 1 if signature is valid, 0 otherwise. */
typedef int (*batch_verify_fn)(void* user_ctx,
                               const uint8_t* msg,
                               size_t msg_len,
                               const uint8_t* sig,
                               size_t sig_len);

/* Create signer context. root_sig_capacity is the max root-signature size. */
batch_signer_t* batch_signer_create(size_t hash_size,
                                    size_t root_sig_capacity,
                                    batch_hash_fn hash,
                                    batch_sign_fn sign,
                                    void* user_ctx);

void batch_signer_free(batch_signer_t* ctx);
int batch_signer_add_message(batch_signer_t* ctx, const uint8_t* msg, size_t msg_len);
void batch_signer_reset(batch_signer_t* ctx);
size_t batch_signer_count(const batch_signer_t* ctx);
batch_signature_t* batch_signer_sign(batch_signer_t* ctx);

/*
 * Serialize to binary format:
 * [u32 version=1][u32 hash_size][u32 num_proofs]
 * [u32 root_hash_len][root_hash]
 * [u32 root_sig_len][root_signature]
 * repeated num_proofs times: [u32 proof_len][proof_hash_0..proof_hash_n-1]
 */
int batch_signature_serialize(const batch_signature_t* sig, uint8_t* out, size_t* out_len);

batch_signature_t* batch_signature_deserialize(const uint8_t* data, size_t data_len,
                                               size_t hash_size);

void batch_signature_free(batch_signature_t* sig);

int batch_verify(const uint8_t* const* messages,
                 const size_t* msg_lens,
                 size_t num_msgs,
                 const batch_signature_t* sig,
                 size_t hash_size,
                 batch_hash_fn hash,
                 batch_verify_fn verify,
                 void* user_ctx);

const uint8_t* batch_signature_root_hash(const batch_signature_t* sig);
const uint8_t* batch_signature_root_signature(const batch_signature_t* sig, size_t* out_len);
size_t batch_signature_num_proofs(const batch_signature_t* sig);

#ifdef __cplusplus
}
#endif

#endif /* BATCH_SIGNING_H */