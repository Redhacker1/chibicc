struct Outer {
    struct Inner { int a, b; } in;
    int c;
};
int main(void) {
    struct Outer o = {{1, 2}, 3};
    return (o.in.a == 1 && o.in.b == 2 && o.c == 3) ? 0 : 1;
}