enum E { A = 10, B = 20 };
int main(void) {
    int x = A;
    enum E e = 20;
    return (x == 10 && e == B) ? 0 : 1;
}