int main(void) {
    int x = 42;
    void *vp = &x;
    int *ip = vp;
    return (*ip == 42) ? 0 : 1;
}