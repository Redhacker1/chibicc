#include <stdlib.h>
_Noreturn void terminate(void) {
    exit(0);
}
int main(void) {
    terminate();
    return 1;
}