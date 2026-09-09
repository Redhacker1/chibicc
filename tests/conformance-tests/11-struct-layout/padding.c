struct Pad {
    char c;
    int i;
};
int main(void) {
    return (sizeof(struct Pad) >= 8) ? 0 : 1;
}