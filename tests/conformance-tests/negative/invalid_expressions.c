int main(void) {
    int x = 10;
    struct S { int a; } s;
    int y = s + x;
    return y;
}