inline static int get_val(void) { return 42; }
int main(void) {
    return (get_val() == 42) ? 0 : 1;
}