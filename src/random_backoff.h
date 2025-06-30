#ifndef RANDOM_BACKOFF_H
#define RANDOM_BACKOFF_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "random/rand64.h"
#include "threading/threading.h"

typedef struct {
    uint64_t min_delay;
    uint64_t max_delay;
    uint64_t limit;
    rand64_gen_t random_gen;
} random_backoff_t;

static inline void random_backoff_init(random_backoff_t *backoff, uint64_t min_delay, uint64_t max_delay) {
    backoff->min_delay = min_delay;
    backoff->max_delay = max_delay;
    backoff->limit = min_delay;
    backoff->random_gen = rand64_gen_init();
    rand64_gen_seed_os(&backoff->random_gen);
}

static inline void random_backoff_reset(random_backoff_t *backoff) {
    backoff->limit = backoff->min_delay;
}

static inline void random_backoff_sleep(random_backoff_t *backoff) {
    uint64_t delay = backoff->min_delay + rand64_gen_bounded(&backoff->random_gen, backoff->limit);
    if (delay > backoff->max_delay) delay = backoff->max_delay;
    uint64_t new_limit = backoff->limit * 2;
    if (new_limit > backoff->max_delay) new_limit = backoff->max_delay;
    backoff->limit = new_limit;
    struct timespec interval;
    interval.tv_sec = delay / 1000000000;
    interval.tv_nsec = delay % 1000000000;
    thrd_sleep(&interval, NULL);
}

#endif