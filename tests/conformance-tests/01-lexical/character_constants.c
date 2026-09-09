int main(void) {
    char c = 'a';
    int esc = '\n';
    int hex = '\x41';
    int oct = '\101';
    return (c == 'a' && esc == 10 && hex == 65 && oct == 65) ? 0 : 1;
}