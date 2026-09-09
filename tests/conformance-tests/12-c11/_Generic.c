#define TYPE_NAME(x) _Generic((x), \
    int: "int", \
    double: "double", \
    char *: "string", \
    default: "other")
int main(void) {
    char *t1 = TYPE_NAME(42);
    char *t2 = TYPE_NAME(3.14);
    char *t3 = TYPE_NAME("hi");
    return (t1[0] == 'i' && t2[0] == 'd' && t3[0] == 's') ? 0 : 1;
}