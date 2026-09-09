struct S {
    _Alignas(16) int x;
};
int main(void) {
    return (sizeof(struct S) >= 16 && _Alignof(struct S) == 16) ? 0 : 1;
}