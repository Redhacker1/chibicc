static int count(void) {
    static int c = 0;
    return ++c;
}
int main(void) {
    count();
    count();
    return (count() == 3) ? 0 : 1;
}