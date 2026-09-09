void add(int * restrict a, const int * restrict b) {
    *a += *b;
}
int main(void) {
    int x = 5;
    int y = 10;
    add(&x, &y);
    return (x == 15) ? 0 : 1;
}