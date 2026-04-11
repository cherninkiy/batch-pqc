#include "gost3411-2012-core.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int digest_is_all_zero(const uint8_t* digest, size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        if (digest[i] != 0u) {
            return 0;
        }
    }
    return 1;
}

static int hash_once(const uint8_t* msg,
                     size_t msg_len,
                     uint8_t* out,
                     unsigned int digest_size_bits) {
    GOST34112012Context ctx;

    GOST34112012Init(&ctx, digest_size_bits);
    GOST34112012Update(&ctx, msg, msg_len);
    GOST34112012Final(&ctx, out);
    GOST34112012Cleanup(&ctx);
    return 0;
}

static int hash_streaming_3_chunks(const uint8_t* msg,
                                   size_t msg_len,
                                   uint8_t* out,
                                   unsigned int digest_size_bits) {
    GOST34112012Context ctx;
    size_t c1 = msg_len / 3u;
    size_t c2 = msg_len / 3u;
    size_t c3 = msg_len - c1 - c2;

    GOST34112012Init(&ctx, digest_size_bits);
    GOST34112012Update(&ctx, msg, c1);
    GOST34112012Update(&ctx, msg + c1, c2);
    GOST34112012Update(&ctx, msg + c1 + c2, c3);
    GOST34112012Final(&ctx, out);
    GOST34112012Cleanup(&ctx);
    return 0;
}

static int test_consistency_for_mode(unsigned int digest_size_bits, size_t digest_len) {
    static const uint8_t msg1[] = "streebog-test-message-1";
    static const uint8_t msg2[] = "streebog-test-message-2";
    uint8_t d1_once[64] = {0};
    uint8_t d1_stream[64] = {0};
    uint8_t d2_once[64] = {0};

    hash_once(msg1, sizeof(msg1) - 1u, d1_once, digest_size_bits);
    hash_streaming_3_chunks(msg1, sizeof(msg1) - 1u, d1_stream, digest_size_bits);
    hash_once(msg2, sizeof(msg2) - 1u, d2_once, digest_size_bits);

    if (memcmp(d1_once, d1_stream, digest_len) != 0) {
        return 1;
    }
    if (memcmp(d1_once, d2_once, digest_len) == 0) {
        return 1;
    }
    if (digest_is_all_zero(d1_once, digest_len)) {
        return 1;
    }

    return 0;
}

int main(void) {
    if (test_consistency_for_mode(256u, 32u) != 0) {
        return 1;
    }
    if (test_consistency_for_mode(512u, 64u) != 0) {
        return 1;
    }

    printf("test_streebog: ok\n");
    return 0;
}
