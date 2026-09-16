
#include "windows_lite.h"
#include "stdbool.h"

#include "chibicc.h"

#ifdef _WIN32
#if defined(_MSC_VER)
#include <process.h>
#include <io.h>
#include <fcntl.h>

unsigned long long __stdcall GetTickCount64(void);

char *strndup(const char *s, size_t n) {
    size_t len = strlen(s);
    if (len > n) len = n;
    char *p = malloc(len + 1);
    if (!p) return NULL;
    memcpy(p, s, len);
    p[len] = '\0';
    return p;
}

int mkstemp(char *tmpl) {
    char *p = strstr(tmpl, "XXXXXX");
    if (!p) return -1;
    static unsigned long counter = 0;
    counter++;
    snprintf(p, 7, "%06lx", (unsigned long)(GetTickCount64() ^ (counter << 12)));
    return _open(tmpl, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
}

int fork(void) {
    return -1;
}

int wait(int *status) {
    (void)status;
    return -1;
}
#endif

#if !defined(_MSC_VER)
__asm__(
".global ___chkstk_ms\n"
"___chkstk_ms:\n"
"  pushq %rcx\n"
"  pushq %rax\n"
"  cmpq $0x1000, %rax\n"
"  lea 24(%rsp), %rcx\n"
"  jb 2f\n"
"1:\n"
"  subq $0x1000, %rcx\n"
"  orl $0, (%rcx)\n"
"  subq $0x1000, %rax\n"
"  cmpq $0x1000, %rax\n"
"  ja 1b\n"
"2:\n"
"  subq %rax, %rcx\n"
"  orl $0, (%rcx)\n"
"  popq %rax\n"
"  popq %rcx\n"
"  ret\n"
);
#endif

#if !defined(_MSC_VER)
int main(const int argc, char **argv);
void exit(int status);

void mainCRTStartup(void) {
  char *cmd = GetCommandLineA();
  int argc = 0;
  int cap = 16;
  char **argv = malloc(sizeof(char *) * cap);

  char *p = cmd;
  while (*p) {
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;

    char *arg_buf = malloc(strlen(p) + 1);
    int len = 0;
    bool in_quotes = false;

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
      argv = realloc(argv, sizeof(char *) * cap);
    }
    argv[argc++] = arg_buf;
  }
  argv[argc] = NULL;

  int ret = main(argc, argv);
  exit(ret);
}
#endif

#if !defined(_MSC_VER)
int stat(const char *path, struct stat *st) {
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(path, &fd);
  if (h == INVALID_HANDLE_VALUE)
    return -1;
  FindClose(h);
  if (st) {
    memset(st, 0, sizeof(*st));
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      st->st_mode = S_IFDIR | 0777;
    else
      st->st_mode = S_IFREG | 0777;
    st->st_size = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
  }
  return 0;
}
#endif

#if !defined(_MSC_VER)
time_t time(time_t *t) {
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  unsigned long long t64 = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  time_t res = (time_t)((t64 - 116444736000000000ULL) / 10000000ULL);
  if (t) *t = res;
  return res;
}
#endif

#if !defined(_MSC_VER)
char *_fullpath(char *absPath, const char *relPath, size_t maxLength) {
  if (GetFullPathNameA(relPath, (DWORD)maxLength, absPath, NULL))
    return absPath;
  return NULL;
}
#endif

#endif