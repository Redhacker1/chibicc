struct Header {
    int len;
    int data[];
};
int main(void) {
    return (sizeof(struct Header) == sizeof(int)) ? 0 : 1;
}