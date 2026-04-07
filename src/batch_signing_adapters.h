#ifndef BATCH_SIGNING_ADAPTERS_H
#define BATCH_SIGNING_ADAPTERS_H

#include "batch_bench.h"
#include "batch_signing.h"

#include <stddef.h>
#include <stdint.h>

typedef struct batch_bb_adapter_ctx {
    const bb_algorithm* algorithm;
    const uint8_t* sk;
    size_t sk_len;
    const uint8_t* pk;
    size_t pk_len;
} batch_bb_adapter_ctx;

int batch_bb_adapter_init(batch_bb_adapter_ctx* ctx,
                          const bb_algorithm* algorithm,
                          const uint8_t* sk,
                          size_t sk_len,
                          const uint8_t* pk,
                          size_t pk_len);

size_t batch_bb_signature_capacity(const batch_bb_adapter_ctx* ctx);

int batch_bb_sign_callback(void* user_ctx,
                           const uint8_t* msg,
                           size_t msg_len,
                           uint8_t* sig,
                           size_t sig_capacity,
                           size_t* sig_len);

int batch_bb_verify_callback(void* user_ctx,
                             const uint8_t* msg,
                             size_t msg_len,
                             const uint8_t* sig,
                             size_t sig_len);

#endif
