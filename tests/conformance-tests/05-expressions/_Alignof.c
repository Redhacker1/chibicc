int main(void) {
    return (_Alignof(int) >= 4 && _Alignof(char) == 1) ? 0 : 1;
}