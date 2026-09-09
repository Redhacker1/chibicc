static inline int add(int a, int b) {
    return a + b;
}
int main(void) {
    return (add(10, 20) == 30) ? 0 : 1;
}