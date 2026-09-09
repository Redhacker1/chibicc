struct Var {
    union {
        int i;
        double d;
    };
    int type;
};
int main(void) {
    struct Var v;
    v.i = 42;
    v.type = 1;
    return (v.i == 42 && v.type == 1) ? 0 : 1;
}