struct AlignTest {
    char c;
    int i;
    double d;
};
int main(void) {
    return (sizeof(struct AlignTest) % 8 == 0) ? 0 : 1;
}