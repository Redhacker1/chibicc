int *get_ptr(void) {
    static int x = 42;
    return &x;
}
int main(void) {
    int *p1 = get_ptr();
    int *p2 = get_ptr();
    return (p1 == p2 && *p1 == 42) ? 0 : 1;
}