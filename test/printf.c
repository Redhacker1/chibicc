#include <stdio.h>

int main(void) {
    printf("Hello, %s!\n", "World");
    printf("Decimal: %d, Hex: 0x%x, Unsigned: %u\n", 42, 255, 12345);
    printf("Char: %c, Percent: %%\n", 'A');
    printf("Padded: %05d, Width: %10s\n", 7, "test");
    printf("OK\n");
    return 0;
}
