void noop(void) {}
int main(void) {
    void *ptr = (void *)0;
    noop();
    return (ptr == 0) ? 0 : 1;
}