int multi_param(int a, int b, int c, int d, int e, int f, int g) {
    return a + b + c + d + e + f + g;
}
int main(void) {
    return (multi_param(1, 2, 3, 4, 5, 6, 7) == 28) ? 0 : 1;
}