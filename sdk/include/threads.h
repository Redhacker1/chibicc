#ifndef _THREADS_H
#define _THREADS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t thrd_t;
typedef int (*thrd_start_t)(void *);

enum {
    thrd_success  = 0,
    thrd_error    = 1,
    thrd_busy     = 2,
    thrd_timeout  = 3,
    thrd_nomem    = 4
};

enum {
    mtx_plain     = 0,
    mtx_recursive = 1,
    mtx_timed     = 2
};

typedef struct {
    void *DebugInfo;
    long LockCount;
    long RecursionCount;
    void *OwningThread;
    void *LockSemaphore;
    uintptr_t SpinCount;
    int initialized;
    int type;
} mtx_t;

typedef struct {
    void *ptr;
} cnd_t;

typedef unsigned long tss_t;
typedef void (*tss_dtor_t)(void *);

#define TSS_DTOR_ITERATIONS 4

typedef struct {
    int done;
    long started;
} once_flag;

#define ONCE_FLAG_INIT { 0, 0 }

int  thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
thrd_t thrd_current(void);
int  thrd_detach(thrd_t thr);
int  thrd_equal(thrd_t thr0, thrd_t thr1);
void thrd_exit(int res) __attribute__((__noreturn__));
int  thrd_join(thrd_t thr, int *res);
int  thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void thrd_yield(void);

int  mtx_init(mtx_t *mutex, int type);
int  mtx_lock(mtx_t *mutex);
int  mtx_timedlock(mtx_t *mutex, const struct timespec *time_point);
int  mtx_trylock(mtx_t *mutex);
int  mtx_unlock(mtx_t *mutex);
void mtx_destroy(mtx_t *mutex);

int  cnd_init(cnd_t *cond);
int  cnd_signal(cnd_t *cond);
int  cnd_broadcast(cnd_t *cond);
int  cnd_wait(cnd_t *cond, mtx_t *mutex);
int  cnd_timedwait(cnd_t *cond, mtx_t *mutex, const struct timespec *time_point);
void cnd_destroy(cnd_t *cond);

int  tss_create(tss_t *key, tss_dtor_t dtor);
void *tss_get(tss_t key);
int  tss_set(tss_t key, void *val);
void tss_delete(tss_t key);

void call_once(once_flag *flag, void (*func)(void));

#ifdef __cplusplus
}
#endif

#endif /* _THREADS_H */
