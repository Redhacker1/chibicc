int g_val = 100;
int get_val(void) { return g_val; }
int main(void) {
    return (get_val() == 100) ? 0 : 1;
}