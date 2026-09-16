//
// Created by donov on 9/15/2026.
//
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#if defined(_WIN32) && !defined(_MSC_VER)
#include <stddef.h>
#include <stdint.h>

void * __stdcall GetCommandLineA(void);
void __stdcall ExitProcess(unsigned int uExitCode);
void exit(int status);
int main(int argc, char **argv);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
size_t strlen(const char *s);

void _pei386_runtime_relocator(void) {}

void mainCRTStartup(void) {
    char *cmd = (char *)GetCommandLineA();
    int argc = 0;
    int cap = 16;
    char **argv = (char **)malloc(sizeof(char *) * cap);

    char *p = cmd ? cmd : "";
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        char *arg_buf = (char *)malloc(strlen(p) + 1);
        int len = 0;
        int in_quotes = 0;

        while (*p) {
            if (*p == '"') {
                in_quotes = !in_quotes;
                p++;
            } else if (!in_quotes && (*p == ' ' || *p == '\t')) {
                break;
            } else if (*p == '\\' && *(p + 1) == '"') {
                arg_buf[len++] = '"';
                p += 2;
            } else {
                arg_buf[len++] = *p++;
            }
        }
        arg_buf[len] = '\0';

        if (argc + 1 >= cap) {
            cap *= 2;
            argv = (char **)realloc(argv, sizeof(char *) * cap);
        }
        argv[argc++] = arg_buf;
    }
    if (argv) {
        argv[argc] = NULL;
    }

    int ret = main(argc, argv);
    exit(ret);
}
#endif
