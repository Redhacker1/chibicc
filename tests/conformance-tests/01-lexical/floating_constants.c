int main(void) {
    double d = 3.14159;
    float f = 1.0f;
    double sci = 1e-3;
    double hex = 0x1.0p0;
    return (d > 3.0 && f == 1.0f && sci < 0.01 && hex == 1.0) ? 0 : 1;
}