struct S { int a; int b; int c; };
int main(void) {
    struct S s = {1, 2};
    return (s.a == 1 && s.b == 2 && s.c == 0) ? 0 : 1;
}