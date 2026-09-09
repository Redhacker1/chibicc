int main(void) {
    char seq[] = "\a\b\f\n\r\t\v\\\'\"\?";
    return (seq[0] == '\a' && seq[1] == '\b' && seq[3] == '\n') ? 0 : 1;
}