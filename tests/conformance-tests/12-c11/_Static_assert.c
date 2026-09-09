int main(void) {
    _Static_assert(sizeof(int) == 4, "int must be 4 bytes");
    _Static_assert(sizeof(char) == 1, "char must be 1 byte");
    return 0;
}