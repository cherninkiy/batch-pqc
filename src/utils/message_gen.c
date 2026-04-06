#include "message_gen.h"

// Deterministic pseudo-random message generator for reproducible benchmarks.
// bb_mix64: SplitMix64 finalizer – mixes bits for good avalanche effect.
// bb_fill_message: fills 'out' with bytes derived from seed and message index.
// Same (seed, index, out_len) always yields identical output, no external storage.

static uint64_t bb_mix64(uint64_t x) {
    // SplitMix64 finalizer constants for deterministic diffusion.
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

void bb_fill_message(uint8_t *out, size_t out_len, uint64_t seed, uint64_t index) {
    uint64_t state;
    size_t i;

    if (out == NULL) {
        return;
    }

    // 0x9e... is the golden-ratio increment used to decorrelate indices.
    state = bb_mix64(seed ^ (index + 0x9e3779b97f4a7c15ULL));
    for (i = 0; i < out_len; ++i) {
        state = bb_mix64(state + i + 1U);
        out[i] = (uint8_t)(state & 0xFFU);
    }
}
