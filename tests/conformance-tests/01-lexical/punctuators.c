int main(void) {
    int a = 1;
    int b = 2;
    int c = (a += b, a *= 2, a ? a : b);
    return (c == 6) ? 0 : 1;
}