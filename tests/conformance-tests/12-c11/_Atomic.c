int main(void) {
    _Atomic int a = 10;
    a++;
    return (a == 11) ? 0 : 1;
}