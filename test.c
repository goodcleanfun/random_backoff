#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "greatest/greatest.h"
#include "threading/threading.h"

#include "random_backoff.h"

#define NUM_THREADS 100
#define NUM_INCREMENTS 1000

int counter = 0;
mtx_t mutex;

static void lock_and_increment(random_backoff_t *backoff) {
    int i = 1;

    while (mtx_trylock(&mutex) == thrd_busy) {
        random_backoff_sleep(backoff);
        i++;
    }
    counter++;
    mtx_unlock(&mutex);
}


int sleep_thread(void *arg) {
    random_backoff_t *backoff = (random_backoff_t *)arg;
    random_backoff_init_thread_locals(backoff);
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        random_backoff_reset(backoff);
        lock_and_increment(backoff);
    }
    return 0;
}

TEST random_backoff_test(void) {
    random_backoff_t *backoff = random_backoff_new(10, 100);
    ASSERT_EQ(mtx_init(&mutex, mtx_plain), thrd_success);
    thrd_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        ASSERT_EQ(thrd_success, thrd_create(&threads[i], sleep_thread, backoff));
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }

    ASSERT_EQ(counter, NUM_THREADS * NUM_INCREMENTS);
    random_backoff_destroy(backoff);

    PASS();
}

// Main test suite
SUITE(random_backoff_tests) {
    RUN_TEST(random_backoff_test);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(random_backoff_tests);

    GREATEST_MAIN_END();
}