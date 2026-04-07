#include "adapters.h"

#include "api.h"

bb_status bb_hypericum_keygen(
    uint8_t *sk,
    size_t sk_capacity,
    size_t *sk_len,
    uint8_t *pk,
    size_t pk_capacity,
    size_t *pk_len) {
    if (sk == NULL || pk == NULL || sk_len == NULL || pk_len == NULL) {
        return BB_BAD_ARG;
    }
    if (sk_capacity < (size_t)CRYPTO_SECRETKEYBYTES ||
        pk_capacity < (size_t)CRYPTO_PUBLICKEYBYTES) {
        return BB_BUFFER_TOO_SMALL;
    }

    if (crypto_sign_keypair(pk, sk) != 0) {
        return BB_INTERNAL;
    }

    *sk_len = (size_t)CRYPTO_SECRETKEYBYTES;
    *pk_len = (size_t)CRYPTO_PUBLICKEYBYTES;
    return BB_OK;
}

bb_status bb_hypericum_sign(
    const uint8_t *sk,
    size_t sk_len,
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    uint8_t *sig,
    size_t sig_capacity,
    size_t *sig_len) {
    unsigned long long out_sig_len = 0;
    size_t required;

    (void)pk;
    (void)pk_len;

    if (sk == NULL || msg == NULL || sig == NULL || sig_len == NULL) {
        return BB_BAD_ARG;
    }
    if (sk_len != (size_t)CRYPTO_SECRETKEYBYTES) {
        return BB_BAD_ARG;
    }

    required = (size_t)CRYPTO_BYTES;
    if (sig_capacity < required) {
        return BB_BUFFER_TOO_SMALL;
    }

    if (crypto_sign_detached(sig, &out_sig_len, msg,
                             (unsigned long long)msg_len, sk) != 0) {
        return BB_INTERNAL;
    }

    if (out_sig_len != (unsigned long long)CRYPTO_BYTES) {
        return BB_INTERNAL;
    }

    *sig_len = (size_t)out_sig_len;
    return BB_OK;
}

bb_status bb_hypericum_verify(
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    const uint8_t *sig,
    size_t sig_len) {
    if (pk == NULL || msg == NULL || sig == NULL) {
        return BB_BAD_ARG;
    }
    if (pk_len != (size_t)CRYPTO_PUBLICKEYBYTES) {
        return BB_BAD_ARG;
    }
    if (sig_len != (size_t)CRYPTO_BYTES) {
        return BB_INVALID_SIG;
    }

    if (crypto_sign_verify_detached(sig, (unsigned long long)sig_len, msg,
                                    (unsigned long long)msg_len, pk) != 0) {
        return BB_INVALID_SIG;
    }

    return BB_OK;
}
