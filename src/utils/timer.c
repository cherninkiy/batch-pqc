#include "timer.h"

#include <time.h>

uint64_t bb_now_monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

void bb_timer_start(bb_timer *timer) {
    if (timer == NULL) {
        return;
    }
    timer->start_ns = bb_now_monotonic_ns();
    timer->end_ns = timer->start_ns;
}

void bb_timer_stop(bb_timer *timer) {
    if (timer == NULL) {
        return;
    }
    timer->end_ns = bb_now_monotonic_ns();
}

uint64_t bb_timer_elapsed_ns(const bb_timer *timer) {
    if (timer == NULL || timer->end_ns < timer->start_ns) {
        return 0;
    }
    return timer->end_ns - timer->start_ns;
}

double bb_timer_elapsed_ms(const bb_timer *timer) {
    return (double)bb_timer_elapsed_ns(timer) / 1000000.0;
}
