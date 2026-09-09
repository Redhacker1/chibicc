struct Container {
    struct {
        int x;
        int y;
    };
    int z;
};
int main(void) {
    struct Container c = {{10, 20}, 30};
    return (c.x == 10 && c.y == 20 && c.z == 30) ? 0 : 1;
}