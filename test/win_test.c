#include <windows.h>
#include <stdio.h>

int main(void) {
    DWORD pid = GetCurrentProcessId();
    if (pid > 0) {
        printf("Process ID: %lu\n", (unsigned long)pid);
        printf("OK\n");
        return 0;
    }
    return 1;
}
