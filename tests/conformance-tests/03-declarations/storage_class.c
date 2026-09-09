static int s_var = 10;
extern int s_var;
int main(void) {
    auto int a = 20;
    register int r = 30;
    return (s_var + a + r == 60) ? 0 : 1;
}