#include "adapters.h"

#include "api.h"

#include <stdlib.h>
#include <string.h>

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
    unsigned char *sm = NULL;
    unsigned long long smlen = 0;
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

    sm = (unsigned char *)malloc(required + msg_len);
    if (sm == NULL) {
        return BB_INTERNAL;
    }

    if (crypto_sign(sm, &smlen, msg, (unsigned long long)msg_len, sk) != 0) {
        free(sm);
        return BB_INTERNAL;
    }

    if (smlen < (unsigned long long)CRYPTO_BYTES) {
        free(sm);
        return BB_INTERNAL;
    }

    memcpy(sig, sm, required);
    *sig_len = required;

    free(sm);
    return BB_OK;
}

bb_status bb_hypericum_verify(
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    const uint8_t *sig,
    size_t sig_len) {
    unsigned char *sm = NULL;
    unsigned char *recovered = NULL;
    unsigned long long recovered_len = 0;
    bb_status status = BB_INVALID_SIG;

    if (pk == NULL || msg == NULL || sig == NULL) {
        return BB_BAD_ARG;
    }
    if (pk_len != (size_t)CRYPTO_PUBLICKEYBYTES) {
        return BB_BAD_ARG;
    }
    if (sig_len != (size_t)CRYPTO_BYTES) {
        return BB_INVALID_SIG;
    }

    sm = (unsigned char *)malloc(sig_len + msg_len);
    recovered = (unsigned char *)malloc(msg_len);
    if (sm == NULL || recovered == NULL) {
        status = BB_INTERNAL;
        goto cleanup;
    }

    memcpy(sm, sig, sig_len);
    memcpy(sm + sig_len, msg, msg_len);

    if (crypto_sign_open(recovered, &recovered_len, sm,
                         (unsigned long long)(sig_len + msg_len), pk) != 0) {
        status = BB_INVALID_SIG;
        goto cleanup;
    }

    if (recovered_len != (unsigned long long)msg_len ||
        memcmp(recovered, msg, msg_len) != 0) {
        status = BB_INVALID_SIG;
        goto cleanup;
    }

    status = BB_OK;

cleanup:
    free(sm);
    free(recovered);
    return status;
}
