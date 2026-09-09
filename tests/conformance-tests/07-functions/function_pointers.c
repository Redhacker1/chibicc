int mul2(int x) { return x * 2; }
int mul3(int x) { return x * 3; }
int main(void) {
    int (*fns[2])(int) = {mul2, mul3};
    return (fns[0](10) == 20 && fns[1](10) == 30) ? 0 : 1;
}