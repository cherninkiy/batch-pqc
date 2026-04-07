#include "batch_bench.h"
#include "utils/message_gen.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_BATCH_SIZE 10

static int test_algorithm_batch(const bb_algorithm *algo) {
    uint8_t *sk = NULL;
    uint8_t *pk = NULL;
    uint8_t *msgs = NULL;
    uint8_t *sigs = NULL;
    size_t *sig_lens = NULL;
    size_t sk_len = 0;
    size_t pk_len = 0;
    size_t i;

    sk = (uint8_t *)malloc(algo->secret_key_bytes);
    pk = (uint8_t *)malloc(algo->public_key_bytes);
    msgs = (uint8_t *)malloc(TEST_BATCH_SIZE * BB_DEFAULT_MESSAGE_SIZE);
    sigs = (uint8_t *)malloc(TEST_BATCH_SIZE * algo->signature_bytes);
    sig_lens = (size_t *)calloc(TEST_BATCH_SIZE, sizeof(size_t));

    if (sk == NULL || pk == NULL || msgs == NULL || sigs == NULL || sig_lens == NULL) {
        free(sk);
        free(pk);
        free(msgs);
        free(sigs);
        free(sig_lens);
        return 1;
    }

    if (algo->keygen(sk, algo->secret_key_bytes, &sk_len,
                     pk, algo->public_key_bytes, &pk_len) != BB_OK) {
        free(sk);
        free(pk);
        free(msgs);
        free(sigs);
        free(sig_lens);
        return 1;
    }

    for (i = 0; i < TEST_BATCH_SIZE; ++i) {
        uint8_t *msg = msgs + i * BB_DEFAULT_MESSAGE_SIZE;
        uint8_t *sig = sigs + i * algo->signature_bytes;

        bb_fill_message(msg, BB_DEFAULT_MESSAGE_SIZE, 7, i);

        if (algo->sign(sk, sk_len, pk, pk_len,
                       msg, BB_DEFAULT_MESSAGE_SIZE,
                       sig, algo->signature_bytes,
                       &sig_lens[i]) != BB_OK) {
            free(sk);
            free(pk);
            free(msgs);
            free(sigs);
            free(sig_lens);
            return 1;
        }

        if (algo->verify(pk, pk_len,
                         msg, BB_DEFAULT_MESSAGE_SIZE,
                         sig, sig_lens[i]) != BB_OK) {
            free(sk);
            free(pk);
            free(msgs);
            free(sigs);
            free(sig_lens);
            return 1;
        }
    }

    for (i = 0; i < TEST_BATCH_SIZE; ++i) {
        uint8_t *msg = msgs + i * BB_DEFAULT_MESSAGE_SIZE;
        uint8_t *sig = sigs + i * algo->signature_bytes;

        msg[0] ^= 0x01U;
        if (algo->verify(pk, pk_len,
                         msg, BB_DEFAULT_MESSAGE_SIZE,
                         sig, sig_lens[i]) == BB_OK) {
            free(sk);
            free(pk);
            free(msgs);
            free(sigs);
            free(sig_lens);
            return 1;
        }
        msg[0] ^= 0x01U;
    }

    free(sk);
    free(pk);
    free(msgs);
    free(sigs);
    free(sig_lens);
    return 0;
}

int main(void) {
    size_t i;

    for (i = 0; i < bb_algorithm_count(); ++i) {
        const bb_algorithm *algo = bb_algorithm_at(i);
        if (test_algorithm_batch(algo) != 0) {
            return 1;
        }
    }

    printf("test_adapters_batch: ok\n");
    return 0;
}
