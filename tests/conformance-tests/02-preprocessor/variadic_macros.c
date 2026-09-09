#define SUM(a, ...) (a + __VA_ARGS__)
int main(void) {
    int x = SUM(10, 20 + 30);
    return (x == 60) ? 0 : 1;
}