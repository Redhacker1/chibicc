struct S;
void use(struct S *s);
struct S { int x; };
int main(void) {
    struct S s = {42};
    return (s.x == 42) ? 0 : 1;
}