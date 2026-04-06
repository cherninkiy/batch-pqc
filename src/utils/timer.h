#ifndef BATCH_BENCH_TIMER_H
#define BATCH_BENCH_TIMER_H

#include <stdint.h>

typedef struct bb_timer {
    uint64_t start_ns;
    uint64_t end_ns;
} bb_timer;

uint64_t bb_now_monotonic_ns(void);
void bb_timer_start(bb_timer *timer);
void bb_timer_stop(bb_timer *timer);
uint64_t bb_timer_elapsed_ns(const bb_timer *timer);
double bb_timer_elapsed_ms(const bb_timer *timer);

#endif
