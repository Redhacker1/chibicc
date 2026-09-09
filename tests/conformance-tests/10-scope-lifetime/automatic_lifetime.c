int get_auto(int val) {
    int x = val;
    return x;
}
int main(void) {
    return (get_auto(10) == 10 && get_auto(20) == 20) ? 0 : 1;
}