struct Node {
    int val;
    struct Node *next;
};
int main(void) {
    struct Node b = {20, (void*)0};
    struct Node a = {10, &b};
    return (a.val == 10 && a.next->val == 20) ? 0 : 1;
}