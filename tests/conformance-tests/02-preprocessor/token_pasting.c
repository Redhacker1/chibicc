#define CONCAT(a, b) a ## b
int main(void) {
    int xy = 123;
    int val = CONCAT(x, y);
    return (val == 123) ? 0 : 1;
}