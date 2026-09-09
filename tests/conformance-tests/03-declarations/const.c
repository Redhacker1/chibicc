int main(void) {
    const int x = 100;
    const int *ptr = &x;
    return (*ptr == 100) ? 0 : 1;
}