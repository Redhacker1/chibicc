int main(void) {
    double d = 42.9;
    int i = (int)d;
    void *p = (void *)(unsigned long)i;
    return (i == 42 && (unsigned long)p == 42) ? 0 : 1;
}