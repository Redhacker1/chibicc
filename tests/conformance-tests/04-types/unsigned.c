int main(void) {
    unsigned char uc = 255;
    unsigned short us = 65535;
    unsigned int ui = 4000000000U;
    return (uc == 255 && us == 65535 && ui > 0) ? 0 : 1;
}