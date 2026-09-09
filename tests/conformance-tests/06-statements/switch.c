int main(void) {
    int v = 2;
    int res = 0;
    switch (v) {
        case 1: res = 10; break;
        case 2: res = 20; break;
        default: res = 30; break;
    }
    return (res == 20) ? 0 : 1;
}