int main(void) {
    int x = 0;
    int y = (x = 5, x + 10);
    return (y == 15) ? 0 : 1;
}