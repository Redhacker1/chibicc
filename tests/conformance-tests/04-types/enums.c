enum Color { RED = 1, GREEN = 2, BLUE = 4 };
int main(void) {
    enum Color c = GREEN | BLUE;
    return (c == 6) ? 0 : 1;
}