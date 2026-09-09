// 01-lexical/identifiers.c
int main(void) {
    int _a = 1;
    int a_1 = 2;
    int $xyz = 3;
    int ABC_def_123 = 4;
    return (_a + a_1 + $xyz + ABC_def_123 == 10) ? 0 : 1;
}
