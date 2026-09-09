int main(void) {
    int side_effect = 0;
    int r = 0 && (++side_effect);
    return (r == 0 && side_effect == 0) ? 0 : 1;
}