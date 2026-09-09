int main(void) {
    int arr[10];
    return (sizeof(char) == 1 && sizeof(arr) == 10 * sizeof(int)) ? 0 : 1;
}