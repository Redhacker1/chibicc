union Data {
    int i;
    char c[4];
};
int main(void) {
    union Data d;
    d.i = 0x01020304;
    return (sizeof(d) == sizeof(int)) ? 0 : 1;
}