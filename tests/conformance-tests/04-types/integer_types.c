int main(void) {
    short s = 1;
    int i = 2;
    long l = 3;
    long long ll = 4;
    return (sizeof(s) <= sizeof(i) && sizeof(i) <= sizeof(l) && sizeof(l) <= sizeof(ll)) ? 0 : 1;
}