#ifndef BATCH_BENCH_MESSAGE_GEN_H
#define BATCH_BENCH_MESSAGE_GEN_H

#include <stddef.h>
#include <stdint.h>

#define BB_DEFAULT_MESSAGE_SIZE 1024

void bb_fill_message(uint8_t *out, size_t out_len, uint64_t seed, uint64_t index);

#endif
