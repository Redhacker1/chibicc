int main(void) {
    int x = 1;
    goto target;
    x = 10;
target:
    return (x == 1) ? 0 : 1;
}