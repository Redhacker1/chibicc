int main(void) {
    int x = 1 ? 42 : 100;
    int y = 0 ? 42 : 100;
    return (x == 42 && y == 100) ? 0 : 1;
}