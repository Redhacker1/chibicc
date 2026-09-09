union U { int x; char c[4]; };
int main(void) {
    union U u = {0x12345678};
    return (u.x == 0x12345678) ? 0 : 1;
}