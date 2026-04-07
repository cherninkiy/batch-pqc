#include "batch_bench.h"
#include "batch_signing.h"
#include "batch_signing_adapters.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE_32 32u
#define MSG_COUNT 4u

typedef struct fake_sig_ctx {
    uint8_t key[HASH_SIZE_32];
} fake_sig_ctx;

static void test_hash32(const uint8_t* in, size_t in_len, uint8_t* out) {
    size_t i;

    memset(out, 0, HASH_SIZE_32);
    for (i = 0; i < in_len; ++i) {
        out[i % HASH_SIZE_32] ^= (uint8_t)(in[i] + (uint8_t)(i * 17u));
    }
    for (i = 0; i < HASH_SIZE_32; ++i) {
        out[i] ^= (uint8_t)(0xA5u + (uint8_t)i);
    }
}

static int fake_sign_cb(void* user_ctx,
                        const uint8_t* msg,
                        size_t msg_len,
                        uint8_t* sig,
                        size_t sig_capacity,
                        size_t* sig_len) {
    fake_sig_ctx* ctx = (fake_sig_ctx*)user_ctx;
    size_t i;

    if (ctx == NULL || msg == NULL || sig == NULL || sig_len == NULL) {
        return 0;
    }
    if (msg_len != HASH_SIZE_32 || sig_capacity < HASH_SIZE_32) {
        return 0;
    }

    for (i = 0; i < HASH_SIZE_32; ++i) {
        sig[i] = msg[i] ^ ctx->key[i];
    }
    *sig_len = HASH_SIZE_32;
    return 1;
}

static int fake_verify_cb(void* user_ctx,
                          const uint8_t* msg,
                          size_t msg_len,
                          const uint8_t* sig,
                          size_t sig_len) {
    fake_sig_ctx* ctx = (fake_sig_ctx*)user_ctx;
    size_t i;

    if (ctx == NULL || msg == NULL || sig == NULL) {
        return 0;
    }
    if (msg_len != HASH_SIZE_32 || sig_len != HASH_SIZE_32) {
        return 0;
    }

    for (i = 0; i < HASH_SIZE_32; ++i) {
        if ((uint8_t)(sig[i] ^ ctx->key[i]) != msg[i]) {
            return 0;
        }
    }
    return 1;
}

static int test_roundtrip_and_tamper(void) {
    static const uint8_t m0[] = "alpha";
    static const uint8_t m1[] = "beta";
    static const uint8_t m2[] = "gamma";

    const uint8_t* messages[3] = {m0, m1, m2};
    size_t lens[3] = {sizeof(m0) - 1u, sizeof(m1) - 1u, sizeof(m2) - 1u};
    fake_sig_ctx ctx;
    batch_signer_t* signer;
    batch_signature_t* sig = NULL;
    batch_signature_t* decoded = NULL;
    uint8_t* blob = NULL;
    size_t blob_len = 0;
    int ok = 0;
    size_t i;

    for (i = 0; i < HASH_SIZE_32; ++i) {
        ctx.key[i] = (uint8_t)(0x10u + i);
    }

    signer = batch_signer_create(HASH_SIZE_32,
                                 HASH_SIZE_32,
                                 test_hash32,
                                 fake_sign_cb,
                                 fake_verify_cb,
                                 &ctx);
    if (signer == NULL) {
        return 1;
    }

    for (i = 0; i < 3u; ++i) {
        if (batch_signer_add_message(signer, messages[i], lens[i]) != 0) {
            batch_signer_free(signer);
            return 1;
        }
    }

    sig = batch_signer_sign(signer);
    if (sig == NULL) {
        batch_signer_free(signer);
        return 1;
    }

    if (!batch_verify(messages, lens, 3u, sig, HASH_SIZE_32, test_hash32, fake_verify_cb, &ctx)) {
        batch_signature_free(sig);
        batch_signer_free(signer);
        return 1;
    }

    if (batch_signature_serialize(sig, NULL, &blob_len) != 0 || blob_len == 0) {
        batch_signature_free(sig);
        batch_signer_free(signer);
        return 1;
    }

    blob = (uint8_t*)malloc(blob_len);
    if (blob == NULL) {
        batch_signature_free(sig);
        batch_signer_free(signer);
        return 1;
    }

    if (batch_signature_serialize(sig, blob, &blob_len) != 0) {
        free(blob);
        batch_signature_free(sig);
        batch_signer_free(signer);
        return 1;
    }

    decoded = batch_signature_deserialize(blob, blob_len, HASH_SIZE_32);
    free(blob);
    if (decoded == NULL) {
        batch_signature_free(sig);
        batch_signer_free(signer);
        return 1;
    }

    ok = batch_verify(messages, lens, 3u, decoded, HASH_SIZE_32, test_hash32, fake_verify_cb, &ctx);
    if (!ok) {
        batch_signature_free(decoded);
        batch_signature_free(sig);
        batch_signer_free(signer);
        return 1;
    }

    {
        size_t root_sig_len = 0;
        const uint8_t* root_sig = batch_signature_root_signature(decoded, &root_sig_len);
        if (root_sig == NULL || root_sig_len == 0) {
            batch_signature_free(decoded);
            batch_signature_free(sig);
            batch_signer_free(signer);
            return 1;
        }
    }

    batch_signature_free(decoded);
    batch_signature_free(sig);
    batch_signer_free(signer);
    return 0;
}

static int test_adapter_integration(void) {
    const bb_algorithm* algo = bb_find_algorithm("hypericum");
    uint8_t* sk = NULL;
    uint8_t* pk = NULL;
    size_t sk_len = 0;
    size_t pk_len = 0;
    uint8_t messages[MSG_COUNT][64];
    const uint8_t* msg_ptrs[MSG_COUNT];
    size_t msg_lens[MSG_COUNT];
    batch_bb_adapter_ctx adapter_ctx;
    batch_signer_t* signer = NULL;
    batch_signature_t* sig = NULL;
    size_t i;

    if (algo == NULL) {
        return 1;
    }

    sk = (uint8_t*)malloc(algo->secret_key_bytes);
    pk = (uint8_t*)malloc(algo->public_key_bytes);
    if (sk == NULL || pk == NULL) {
        free(sk);
        free(pk);
        return 1;
    }

    if (algo->keygen(sk,
                     algo->secret_key_bytes,
                     &sk_len,
                     pk,
                     algo->public_key_bytes,
                     &pk_len) != BB_OK) {
        free(sk);
        free(pk);
        return 1;
    }

    if (batch_bb_adapter_init(&adapter_ctx, algo, sk, sk_len, pk, pk_len) != 0) {
        free(sk);
        free(pk);
        return 1;
    }

    signer = batch_signer_create(HASH_SIZE_32,
                                 batch_bb_signature_capacity(&adapter_ctx),
                                 test_hash32,
                                 batch_bb_sign_callback,
                                 batch_bb_verify_callback,
                                 &adapter_ctx);
    if (signer == NULL) {
        free(sk);
        free(pk);
        return 1;
    }

    for (i = 0; i < MSG_COUNT; ++i) {
        size_t j;
        for (j = 0; j < sizeof(messages[i]); ++j) {
            messages[i][j] = (uint8_t)(i + j + 1u);
        }
        msg_ptrs[i] = messages[i];
        msg_lens[i] = sizeof(messages[i]);
        if (batch_signer_add_message(signer, messages[i], sizeof(messages[i])) != 0) {
            batch_signer_free(signer);
            free(sk);
            free(pk);
            return 1;
        }
    }

    sig = batch_signer_sign(signer);
    if (sig == NULL) {
        batch_signer_free(signer);
        free(sk);
        free(pk);
        return 1;
    }

    if (!batch_verify(msg_ptrs,
                      msg_lens,
                      MSG_COUNT,
                      sig,
                      HASH_SIZE_32,
                      test_hash32,
                      batch_bb_verify_callback,
                      &adapter_ctx)) {
        batch_signature_free(sig);
        batch_signer_free(signer);
        free(sk);
        free(pk);
        return 1;
    }

    batch_signature_free(sig);
    batch_signer_free(signer);
    free(sk);
    free(pk);
    return 0;
}

int main(void) {
    if (test_roundtrip_and_tamper() != 0) {
        return 1;
    }
    if (test_adapter_integration() != 0) {
        return 1;
    }

    printf("test_batch_signing: ok\n");
    return 0;
}
