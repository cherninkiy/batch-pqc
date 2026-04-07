#include "adapters.h"

#include "shipovnik.h"

bb_status bb_shipovnik_keygen(
    uint8_t *sk,
    size_t sk_capacity,
    size_t *sk_len,
    uint8_t *pk,
    size_t pk_capacity,
    size_t *pk_len) {
    if (sk == NULL || pk == NULL || sk_len == NULL || pk_len == NULL) {
        return BB_BAD_ARG;
    }
    if (sk_capacity < (size_t)SHIPOVNIK_SECRETKEYBYTES ||
        pk_capacity < (size_t)SHIPOVNIK_PUBLICKEYBYTES) {
        return BB_BUFFER_TOO_SMALL;
    }

    if (shipovnik_generate_keys_ex(sk, pk) != 0) {
        return BB_INTERNAL;
    }
    *sk_len = (size_t)SHIPOVNIK_SECRETKEYBYTES;
    *pk_len = (size_t)SHIPOVNIK_PUBLICKEYBYTES;
    return BB_OK;
}

bb_status bb_shipovnik_sign(
    const uint8_t *sk,
    size_t sk_len,
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    uint8_t *sig,
    size_t sig_capacity,
    size_t *sig_len) {
    size_t out_sig_len = 0;

    (void)pk;
    (void)pk_len;

    if (sk == NULL || msg == NULL || sig == NULL || sig_len == NULL) {
        return BB_BAD_ARG;
    }
    if (sk_len != (size_t)SHIPOVNIK_SECRETKEYBYTES) {
        return BB_BAD_ARG;
    }
    if (sig_capacity < (size_t)SHIPOVNIK_SIGBYTES) {
        return BB_BUFFER_TOO_SMALL;
    }

    if (shipovnik_sign_ex(sk, msg, msg_len, sig, &out_sig_len) != 0) {
        return BB_INTERNAL;
    }
    if (out_sig_len > (size_t)SHIPOVNIK_SIGBYTES) {
        return BB_INTERNAL;
    }
    *sig_len = out_sig_len;
    return BB_OK;
}

bb_status bb_shipovnik_verify(
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    const uint8_t *sig,
    size_t sig_len) {
    if (pk == NULL || msg == NULL || sig == NULL) {
        return BB_BAD_ARG;
    }
    if (pk_len != (size_t)SHIPOVNIK_PUBLICKEYBYTES) {
        return BB_BAD_ARG;
    }
    if (sig_len > (size_t)SHIPOVNIK_SIGBYTES) {
        return BB_INVALID_SIG;
    }

    return shipovnik_verify(pk, sig, msg, msg_len) == 0 ? BB_OK : BB_INVALID_SIG;
}
