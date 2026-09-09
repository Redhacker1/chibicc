int main(void) {
    int sum = 0;
    for (int i = 1; i <= 4; i++) sum += i;
    return (sum == 10) ? 0 : 1;
}