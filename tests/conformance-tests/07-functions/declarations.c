int foo(int, int);
int foo(int a, int b) { return a + b; }
int main(void) {
    return (foo(20, 22) == 42) ? 0 : 1;
}