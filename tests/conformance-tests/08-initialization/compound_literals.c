struct Point { int x, y; };
int main(void) {
    struct Point *p = &(struct Point){10, 20};
    int *arr = (int[]){1, 2, 3};
    return (p->x == 10 && p->y == 20 && arr[2] == 3) ? 0 : 1;
}