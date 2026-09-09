typedef int (*fn_t)(int);
int inc(int x) { return x + 1; }
int main(void) {
    fn_t f = inc;
    return (f(41) == 42) ? 0 : 1;
}