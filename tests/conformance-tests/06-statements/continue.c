int main(void) {
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (i % 2 == 0) continue;
        count++;
    }
    return (count == 2) ? 0 : 1;
}