int main(void) {
    int x = 5;
    int y = -x;
    int z = ~x;
    int not_x = !x;
    return (y == -5 && z == -6 && not_x == 0) ? 0 : 1;
}