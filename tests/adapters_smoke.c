#include "batch_bench.h"
#include "utils/message_gen.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    size_t i;
    uint8_t msg[BB_DEFAULT_MESSAGE_SIZE];

    bb_fill_message(msg, sizeof(msg), 1, 0);

    for (i = 0; i < bb_algorithm_count(); ++i) {
        const bb_algorithm *algo = bb_algorithm_at(i);
        uint8_t *sk = (uint8_t *)malloc(algo->secret_key_bytes);
        uint8_t *pk = (uint8_t *)malloc(algo->public_key_bytes);
        uint8_t *sig = (uint8_t *)malloc(algo->signature_bytes);
        size_t sk_len = 0;
        size_t pk_len = 0;
        size_t sig_len = 0;

        if (sk == NULL || pk == NULL || sig == NULL) {
            free(sk);
            free(pk);
            free(sig);
            return 1;
        }

        if (algo->keygen(sk, algo->secret_key_bytes, &sk_len,
                         pk, algo->public_key_bytes, &pk_len) != BB_OK) {
            free(sk);
            free(pk);
            free(sig);
            return 1;
        }

        if (algo->sign(sk, sk_len, pk, pk_len, msg, sizeof(msg),
                       sig, algo->signature_bytes, &sig_len) != BB_OK) {
            free(sk);
            free(pk);
            free(sig);
            return 1;
        }

        if (algo->verify(pk, pk_len, msg, sizeof(msg), sig, sig_len) != BB_OK) {
            free(sk);
            free(pk);
            free(sig);
            return 1;
        }

        msg[0] ^= 0x01U;
        if (algo->verify(pk, pk_len, msg, sizeof(msg), sig, sig_len) == BB_OK) {
            free(sk);
            free(pk);
            free(sig);
            return 1;
        }
        msg[0] ^= 0x01U;

        free(sk);
        free(pk);
        free(sig);
    }

    printf("adapters_smoke: ok\n");
    return 0;
}
