int main(void) {
    char s1[] = "hello";
    char s2[5] = "world";
    return (s1[4] == 'o' && s1[5] == '\0' && s2[4] == 'd') ? 0 : 1;
}