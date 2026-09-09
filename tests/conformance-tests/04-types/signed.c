int main(void) {
    signed char sc = -1;
    signed short ss = -2;
    signed int si = -3;
    signed long sl = -4;
    return (sc < 0 && ss < 0 && si < 0 && sl < 0) ? 0 : 1;
}