#include "../batch_bench.h"
#include "adapters.h"

#include "api.h"
#include "shipovnik.h"
#include "kryzhovnik_wrapper.h"

#include <string.h>

static const bb_algorithm k_algorithms[] = {
    {
        .name = "hypericum_m_128_20",
        .public_key_bytes = CRYPTO_PUBLICKEYBYTES,
        .secret_key_bytes = CRYPTO_SECRETKEYBYTES,
        .signature_bytes = CRYPTO_BYTES,
        .keygen = bb_hypericum_keygen,
        .sign = bb_hypericum_sign,
        .verify = bb_hypericum_verify,
    },
    {
        .name = "shipovnik",
        .public_key_bytes = SHIPOVNIK_PUBLICKEYBYTES,
        .secret_key_bytes = SHIPOVNIK_SECRETKEYBYTES,
        .signature_bytes = SHIPOVNIK_SIGBYTES,
        .keygen = bb_shipovnik_keygen,
        .sign = bb_shipovnik_sign,
        .verify = bb_shipovnik_verify,
    },
    {
        .name = "kryzhovnik",
        .public_key_bytes = KRYZHOVNIK_PUBLIC_KEY_BYTES,
        .secret_key_bytes = KRYZHOVNIK_SECRET_KEY_BYTES,
        .signature_bytes = KRYZHOVNIK_SIGNATURE_BYTES,
        .keygen = bb_kryzhovnik_keygen,
        .sign = bb_kryzhovnik_sign,
        .verify = bb_kryzhovnik_verify,
    },
};

size_t bb_algorithm_count(void) {
    return sizeof(k_algorithms) / sizeof(k_algorithms[0]);
}

const bb_algorithm *bb_algorithm_at(size_t index) {
    if (index >= bb_algorithm_count()) {
        return NULL;
    }
    return &k_algorithms[index];
}

const bb_algorithm *bb_find_algorithm(const char *name) {
    size_t i;

    if (name == NULL) {
        return NULL;
    }

    for (i = 0; i < bb_algorithm_count(); ++i) {
        if (strcmp(k_algorithms[i].name, name) == 0) {
            return &k_algorithms[i];
        }
    }

    return NULL;
}
