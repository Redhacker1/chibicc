struct Pt { int x, y, z; };
int main(void) {
    struct Pt p = {.y = 20, .x = 10};
    int arr[5] = {[1] = 2, [4] = 5};
    return (p.x == 10 && p.y == 20 && p.z == 0 && arr[0] == 0 && arr[1] == 2 && arr[4] == 5) ? 0 : 1;
}