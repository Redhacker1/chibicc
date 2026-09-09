int main(void) {
    return (_Alignof(double) >= 8 && _Alignof(char) == 1) ? 0 : 1;
}