int main(void) {
    float f = 1.0f;
    double d = 2.0;
    long double ld = 3.0L;
    return (sizeof(f) <= sizeof(d) && sizeof(d) <= sizeof(ld)) ? 0 : 1;
}