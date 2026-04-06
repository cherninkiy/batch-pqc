#ifndef BATCH_BENCH_H
#define BATCH_BENCH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bb_status {
    BB_OK = 0,
    BB_INVALID_SIG = 1,
    BB_BAD_ARG = 2,
    BB_BUFFER_TOO_SMALL = 3,
    BB_INTERNAL = 4,
} bb_status;

typedef struct bb_algorithm {
    const char *name;
    size_t public_key_bytes;
    size_t secret_key_bytes;
    size_t signature_bytes;

    bb_status (*keygen)(
        uint8_t *sk,
        size_t sk_capacity,
        size_t *sk_len,
        uint8_t *pk,
        size_t pk_capacity,
        size_t *pk_len);

    bb_status (*sign)(
        const uint8_t *sk,
        size_t sk_len,
        const uint8_t *pk,
        size_t pk_len,
        const uint8_t *msg,
        size_t msg_len,
        uint8_t *sig,
        size_t sig_capacity,
        size_t *sig_len);

    bb_status (*verify)(
        const uint8_t *pk,
        size_t pk_len,
        const uint8_t *msg,
        size_t msg_len,
        const uint8_t *sig,
        size_t sig_len);
} bb_algorithm;

size_t bb_algorithm_count(void);
const bb_algorithm *bb_algorithm_at(size_t index);
const bb_algorithm *bb_find_algorithm(const char *name);

#ifdef __cplusplus
}
#endif

#endif
