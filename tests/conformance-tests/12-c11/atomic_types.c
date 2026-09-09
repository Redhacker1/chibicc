#include <stdatomic.h>
int main(void) {
    atomic_int a = 20;
    atomic_fetch_add(&a, 5);
    return (a == 25) ? 0 : 1;
}