#include <stddef.h>
#include <stdbool.h>
int main(void) {
    bool b = true;
    size_t sz = sizeof(b);
    return (b && sz >= 1) ? 0 : 1;
}