int main(void) {
    char *s = "hello" " " "world";
    return (s[0] == 'h' && s[5] == ' ' && s[6] == 'w' && s[11] == '\0') ? 0 : 1;
}