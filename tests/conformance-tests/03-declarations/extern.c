extern int g_ext;
int g_ext = 55;
int main(void) {
    return (g_ext == 55) ? 0 : 1;
}