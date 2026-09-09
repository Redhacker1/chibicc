#define FEATURE 1
#if defined(FEATURE) && FEATURE == 1
#define VALUE 42
#elif defined(OTHER)
#define VALUE 0
#else
#define VALUE -1
#endif
int main(void) {
    return (VALUE == 42) ? 0 : 1;
}