#define STR(x) #x
int main(void) {
    char *s = STR(hello world);
    return (s[0] == 'h' && s[5] == ' ') ? 0 : 1;
}