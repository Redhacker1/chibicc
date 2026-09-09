int main(void) {
    int x = 0;
lbl1:
    x++;
    if (x < 3) goto lbl1;
    return (x == 3) ? 0 : 1;
}