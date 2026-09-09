int test_proto(int n, int arr[5]);
int test_proto(int n, int arr[5]) {
    return n + arr[0];
}
int main(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    return (test_proto(5, arr) == 15) ? 0 : 1;
}