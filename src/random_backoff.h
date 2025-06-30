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

static inline random_backoff_t *random_backoff_new(uint64_t min_delay, uint64_t max_delay) {
    random_backoff_t *backoff = (random_backoff_t *)malloc(sizeof(random_backoff_t));
    backoff->min_delay = min_delay;
    backoff->max_delay = max_delay;
    backoff->limit = min_delay;
    backoff->random_gen = PCG64_INITIALIZER;
    rand64_gen_seed_os(&backoff->random_gen);
    return backoff;
}

static inline void random_backkoff_destroy(random_backoff_t *backoff) {
    if (backoff == NULL) return;
    free(backoff);
}

static inline void random_backoff_sleep(random_backoff_t *backoff) {
    uint64_t delay = rand64_gen_bounded(&backoff->random_gen, backoff->limit);
    uint64_t new_limit = delay * 2;
    if (new_limit > backoff->max_delay) new_limit = backoff->max_delay;
    backoff->limit = new_limit;
    struct timespec interval;
    interval.tv_sec = delay / 1000000000;
    interval.tv_nsec = delay % 1000000000;
    thrd_sleep(&interval, NULL);
}

#endif