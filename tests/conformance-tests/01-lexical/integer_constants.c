// 01-lexical/integer_constants.c
int main(void) {
    int dec = 42;
    int oct = 052;
    int hex = 0x2A;
    int bin = 0b101010;
    unsigned int u = 42u;
    long l = 42L;
    long long ll = 42LL;
    unsigned long long ull = 42ULL;
    return (dec == 42 && oct == 42 && hex == 42 && bin == 42 &&
            u == 42 && l == 42 && ll == 42 && ull == 42) ? 0 : 1;
}
