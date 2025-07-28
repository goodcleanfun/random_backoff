#ifndef RANDOM_BACKOFF_H
#define RANDOM_BACKOFF_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "random/rand_u64.h"
#include "threading/threading.h"

typedef struct {
    uint64_t min_delay;
    uint64_t max_delay;
    tss_t limit_tls;
    tss_t random_gen_tls;
} random_backoff_t;

static inline bool random_backoff_init(random_backoff_t *backoff, uint64_t min_delay, uint64_t max_delay) {
    backoff->min_delay = min_delay;
    backoff->max_delay = max_delay;
    return tss_create(&backoff->random_gen_tls, free) == thrd_success &&
           tss_create(&backoff->limit_tls, free) == thrd_success;
}

static inline random_backoff_t *random_backoff_new(uint64_t min_delay, uint64_t max_delay) {
    random_backoff_t *backoff = (random_backoff_t *)malloc(sizeof(random_backoff_t));
    if (!random_backoff_init(backoff, min_delay, max_delay)) {
        free(backoff);
        return NULL;
    }
    return backoff;
}

static inline void random_backoff_destroy(random_backoff_t *backoff) {
    if (backoff == NULL) return;
    free(backoff);
}

static inline void random_backoff_init_thread_local_rng(random_backoff_t *backoff) {
    rand_u64_t *random_gen = (rand_u64_t *)tss_get(backoff->random_gen_tls);
    if (random_gen == NULL) {
        random_gen = (rand_u64_t *)malloc(sizeof(rand_u64_t));
        rand_u64_init(random_gen);
        tss_set(backoff->random_gen_tls, random_gen);
    }
}

static inline void random_backoff_init_thread_local_rng_seed(random_backoff_t *backoff, uint64_t seed) {
    rand_u64_t *random_gen = (rand_u64_t *)tss_get(backoff->random_gen_tls);
    if (random_gen == NULL) {
        random_gen = (rand_u64_t *)malloc(sizeof(rand_u64_t));
        rand_u64_init_seed(random_gen, seed);
        tss_set(backoff->random_gen_tls, random_gen);
    }
}

static inline void random_backoff_init_thread_local_limit(random_backoff_t *backoff) {
    uint64_t *limit = (uint64_t *)tss_get(backoff->limit_tls);
    if (limit == NULL) {
        limit = (uint64_t *)malloc(sizeof(uint64_t));
        *limit = backoff->min_delay;
        tss_set(backoff->limit_tls, limit);
    }
}

static inline void random_backoff_init_thread_locals(random_backoff_t *backoff) {
    random_backoff_init_thread_local_rng(backoff);
    random_backoff_init_thread_local_limit(backoff);
}

static inline void random_backoff_reset(random_backoff_t *backoff) {
    uint64_t *limit = (uint64_t *)tss_get(backoff->limit_tls);
    if (limit == NULL) {
        random_backoff_init_thread_local_limit(backoff);
        limit = (uint64_t *)tss_get(backoff->limit_tls);
    }
    *limit = backoff->min_delay;
}

static inline void random_backoff_sleep(random_backoff_t *backoff) {
    rand_u64_t *random_gen = (rand_u64_t *)tss_get(backoff->random_gen_tls);
    if (random_gen == NULL) {
        random_backoff_init_thread_local_rng(backoff);
    }
    uint64_t *limit_tls = (uint64_t *)tss_get(backoff->limit_tls);
    if (limit_tls == NULL) {
        random_backoff_init_thread_local_limit(backoff);
        limit_tls = (uint64_t *)tss_get(backoff->limit_tls);
    }
    uint64_t limit = *limit_tls;

    uint64_t delay = backoff->min_delay + rand_u64_bounded(random_gen, limit);
    if (delay > backoff->max_delay) delay = backoff->max_delay;
    uint64_t new_limit = limit * 2;
    if (new_limit > backoff->max_delay) new_limit = backoff->max_delay;
    *limit_tls = new_limit;
    struct timespec interval;
    interval.tv_sec = delay / 1000000000;
    interval.tv_nsec = delay % 1000000000;
    thrd_sleep(&interval, NULL);
}

#endif