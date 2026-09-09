int main(void) {
    int x = 10;
    const int *cp = &x;
    return (*cp == 10) ? 0 : 1;
}