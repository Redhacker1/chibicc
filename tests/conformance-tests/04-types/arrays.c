int main(void) {
    int arr[3] = {10, 20, 30};
    int mat[2][2] = {{1, 2}, {3, 4}};
    return (arr[1] == 20 && mat[1][0] == 3) ? 0 : 1;
}