int main(void) {
    volatile int v = 7;
    v = v + 3;
    return (v == 10) ? 0 : 1;
}