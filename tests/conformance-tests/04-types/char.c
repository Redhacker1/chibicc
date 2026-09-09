int main(void) {
    char c = 120;
    return (sizeof(c) == 1 && c == 120) ? 0 : 1;
}