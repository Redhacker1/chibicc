int main(void) {
    int arr[] = {10, 20};
    int i = 0;
    int val = arr[i++];
    return (val == 10 && i == 1) ? 0 : 1;
}