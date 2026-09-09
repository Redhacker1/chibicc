int fact(int n) {
    return (n <= 1) ? 1 : n * fact(n - 1);
}
int main(void) {
    return (fact(5) == 120) ? 0 : 1;
}