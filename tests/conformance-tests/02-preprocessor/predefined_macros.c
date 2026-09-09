int main(void) {
    int line = __LINE__;
    char *file = __FILE__;
    return (line > 0 && file != 0 && __STDC_VERSION__ >= 199901L) ? 0 : 1;
}