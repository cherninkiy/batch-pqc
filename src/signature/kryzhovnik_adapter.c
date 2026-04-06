#include "adapters.h"

#include "kryzhovnik_wrapper.h"

bb_status bb_kryzhovnik_keygen(
    uint8_t *sk,
    size_t sk_capacity,
    size_t *sk_len,
    uint8_t *pk,
    size_t pk_capacity,
    size_t *pk_len) {
    if (sk == NULL || pk == NULL || sk_len == NULL || pk_len == NULL) {
        return BB_BAD_ARG;
    }
    if (sk_capacity < (size_t)KRYZHOVNIK_SECRET_KEY_BYTES ||
        pk_capacity < (size_t)KRYZHOVNIK_PUBLIC_KEY_BYTES) {
        return BB_BUFFER_TOO_SMALL;
    }

    kryzhovnik_generate_keys(sk, pk);
    *sk_len = (size_t)KRYZHOVNIK_SECRET_KEY_BYTES;
    *pk_len = (size_t)KRYZHOVNIK_PUBLIC_KEY_BYTES;
    return BB_OK;
}

bb_status bb_kryzhovnik_sign(
    const uint8_t *sk,
    size_t sk_len,
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    uint8_t *sig,
    size_t sig_capacity,
    size_t *sig_len) {
    if (sk == NULL || pk == NULL || msg == NULL || sig == NULL || sig_len == NULL) {
        return BB_BAD_ARG;
    }
    if (sk_len != (size_t)KRYZHOVNIK_SECRET_KEY_BYTES ||
        pk_len != (size_t)KRYZHOVNIK_PUBLIC_KEY_BYTES) {
        return BB_BAD_ARG;
    }
    if (sig_capacity < (size_t)KRYZHOVNIK_SIGNATURE_BYTES) {
        return BB_BUFFER_TOO_SMALL;
    }

    kryzhovnik_sign(sk, pk, msg, msg_len, sig, sig_len);
    return BB_OK;
}

bb_status bb_kryzhovnik_verify(
    const uint8_t *pk,
    size_t pk_len,
    const uint8_t *msg,
    size_t msg_len,
    const uint8_t *sig,
    size_t sig_len) {
    if (pk == NULL || msg == NULL || sig == NULL) {
        return BB_BAD_ARG;
    }
    if (pk_len != (size_t)KRYZHOVNIK_PUBLIC_KEY_BYTES) {
        return BB_BAD_ARG;
    }
    if (sig_len != (size_t)KRYZHOVNIK_SIGNATURE_BYTES) {
        return BB_INVALID_SIG;
    }

    return kryzhovnik_verify(pk, sig, msg, msg_len) == 0 ? BB_OK : BB_INVALID_SIG;
}
