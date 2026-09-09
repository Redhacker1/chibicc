int main(void) {
    int side_effect = 0;
    int r = 1 || (++side_effect);
    return (r == 1 && side_effect == 0) ? 0 : 1;
}