#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <threads.h>
#include <stdarg.h>

#define ASSERT(expr) do { \
    if (!(expr)) { \
        printf("FAIL: %s (line %d)\n", #expr, __LINE__); \
        exit(1); \
    } \
} while (0)

// Test POSIX threads & mutex
static pthread_mutex_t p_mtx = PTHREAD_MUTEX_INITIALIZER;
static int p_counter = 0;

static void *posix_thread_fn(void *arg) {
    int id = (int)(intptr_t)arg;
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&p_mtx);
        p_counter++;
        pthread_mutex_unlock(&p_mtx);
    }
    return (void *)(intptr_t)(id * 10);
}

// Test C11 threads & mtx & cnd
static mtx_t c_mtx;
static cnd_t c_cnd;
static int c_ready = 0;
static int c_val = 0;

static int c11_worker_fn(void *arg) {
    (void)arg;
    mtx_lock(&c_mtx);
    while (!c_ready) {
        cnd_wait(&c_cnd, &c_mtx);
    }
    c_val = 42;
    mtx_unlock(&c_mtx);
    return 100;
}

// Test call_once / pthread_once
static once_flag oflag = ONCE_FLAG_INIT;
static int once_count = 0;
static void once_init(void) {
    once_count++;
}

// Test __vstrfmon
extern ssize_t __vstrfmon(char *s, size_t maxsize, const char *format, ...);
static ssize_t my_strfmon(char *s, size_t maxsize, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    ssize_t ret = __vstrfmon(s, maxsize, format, ap);
    va_end(ap);
    return ret;
}

int main(void) {
    printf("Testing POSIX threads...\n");
    pthread_t t1, t2;
    int r1 = pthread_create(&t1, NULL, posix_thread_fn, (void *)(intptr_t)1);
    int r2 = pthread_create(&t2, NULL, posix_thread_fn, (void *)(intptr_t)2);
    ASSERT(r1 == 0);
    ASSERT(r2 == 0);

    void *ret1 = NULL;
    void *ret2 = NULL;
    pthread_join(t1, &ret1);
    pthread_join(t2, &ret2);
    ASSERT((int)(intptr_t)ret1 == 10);
    ASSERT((int)(intptr_t)ret2 == 20);
    ASSERT(p_counter == 200);

    printf("Testing C11 threads, mutex and condition variables...\n");
    mtx_init(&c_mtx, mtx_plain);
    cnd_init(&c_cnd);

    thrd_t ct;
    int cr = thrd_create(&ct, c11_worker_fn, NULL);
    ASSERT(cr == thrd_success);

    // Sleep briefly then signal
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000 };
    thrd_sleep(&ts, NULL);

    mtx_lock(&c_mtx);
    c_ready = 1;
    cnd_signal(&c_cnd);
    mtx_unlock(&c_mtx);

    int c_res = 0;
    thrd_join(ct, &c_res);
    ASSERT(c_res == 100);
    ASSERT(c_val == 42);

    mtx_destroy(&c_mtx);
    cnd_destroy(&c_cnd);

    printf("Testing call_once...\n");
    call_once(&oflag, once_init);
    call_once(&oflag, once_init);
    ASSERT(once_count == 1);

    printf("Testing __vstrfmon...\n");
    char mon_buf[128];
    ssize_t n = my_strfmon(mon_buf, sizeof(mon_buf), "%n", 1234.56);
    ASSERT(n > 0);
    ASSERT(strstr(mon_buf, "1234.56") != NULL);
    ASSERT(strchr(mon_buf, '$') != NULL);

    n = my_strfmon(mon_buf, sizeof(mon_buf), "%i", 1234.56);
    ASSERT(n > 0);
    ASSERT(strstr(mon_buf, "USD") != NULL);

    printf("All thread and monetary tests PASSED successfully!\n");
    return 0;
}
