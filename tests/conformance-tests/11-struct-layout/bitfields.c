struct Bits {
    unsigned int a : 3;
    unsigned int b : 5;
};
int main(void) {
    struct Bits b = {7, 31};
    return (b.a == 7 && b.b == 31 && sizeof(b) == sizeof(unsigned int)) ? 0 : 1;
}