typedef unsigned long ulong_t;
typedef struct Point { int x, y; } Point_t;
int main(void) {
    ulong_t u = 100;
    Point_t pt = {10, 20};
    return (u == 100 && pt.x == 10 && pt.y == 20) ? 0 : 1;
}