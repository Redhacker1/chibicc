#define ADD(a, b) ((a) + (b))
#define SQUARE(x) ((x) * (x))
int main(void) {
    return (ADD(3, 4) == 7 && SQUARE(5) == 25) ? 0 : 1;
}