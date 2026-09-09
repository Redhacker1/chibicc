int main(void) {
    int i = -1;
    unsigned int u = 1;
    return (i > u) ? 0 : 1; // -1 promoted to unsigned max
}