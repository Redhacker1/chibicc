struct Ret { int a; int b; };
struct Ret make_ret(int x, int y) {
    struct Ret r = {x, y};
    return r;
}
int main(void) {
    struct Ret r = make_ret(10, 20);
    return (r.a == 10 && r.b == 20) ? 0 : 1;
}