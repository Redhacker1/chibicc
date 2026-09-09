struct Flex {
    int len;
    char data[];
};
int main(void) {
    return (sizeof(struct Flex) == sizeof(int)) ? 0 : 1;
}