int main(void) {
    _Bool b1 = 0;
    _Bool b2 = 100;
    return (b1 == 0 && b2 == 1 && sizeof(_Bool) == 1) ? 0 : 1;
}