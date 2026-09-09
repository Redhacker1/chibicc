#define A(x) B(x)
#define B(x) ((x) + 1)
int main(void) {
    return (A(10) == 11) ? 0 : 1;
}