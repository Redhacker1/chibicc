int main(void) {
    int shl = 1 << 4;
    int shr = 32 >> 2;
    return (shl == 16 && shr == 8) ? 0 : 1;
}