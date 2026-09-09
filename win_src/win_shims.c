
#include "Windows.h"
#include "stdbool.h"

#include "chibicc.h"

#ifdef _WIN32
void error(char *fmt, ...);

char *create_tmpfile(void)
{
#ifdef _WIN32
  char temp_path[MAX_PATH];
  char temp_file[MAX_PATH];

  DWORD len = GetTempPathA(sizeof(temp_path), temp_path);
  if (len == 0 || len >= sizeof(temp_path))
    error("GetTempPath failed");

  if (GetTempFileNameA(temp_path, "cc", 0, temp_file) == 0)
    error("GetTempFileName failed");

  char *path = strdup(temp_file);
  if (!path)
    error("out of memory");

  strarray_push(&tmpfiles, path);
  return path;

#else
  char *path = strdup("/tmp/chibicc-XXXXXX");

  int fd = mkstemp(path);

  if (fd == -1)
    error("mkstemp failed: %s", strerror(errno));

  close(fd);

  strarray_push(&tmpfiles, path);
  return path;
#endif
}

// Windows Subprocess implementation.
bool run_subprocess(char **argv)
{
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);

    // Calculate the required command-line length.
    size_t len = 0;

    for (int i = 0; argv[i]; i++) {
        if (i > 0)
            len++; // Space

        // Leave room for quotes around every argument.
        len += strlen(argv[i]) + 2;
    }

    char *command_line = calloc(1, len + 1);
    if (!command_line)
        error("out of memory");

    // Build the command line.
    char *p = command_line;

    for (int i = 0; argv[i]; i++) {
        if (i > 0)
            *p++ = ' ';

        *p++ = '"';

        size_t arglen = strlen(argv[i]);
        memcpy(p, argv[i], arglen);
        p += arglen;

        *p++ = '"';
    }

    *p = '\0';

    BOOL ok = CreateProcessA(
        NULL,
        command_line,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!ok) {
        fprintf(stderr, "CreateProcess failed: %lu for command: %s\n", GetLastError(), command_line);
        free(command_line);
        return false;
    }

    free(command_line);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(pi.hProcess, &exit_code))
        exit_code = 1;

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (exit_code != 0)
        exit(exit_code);

    return true;
}

#endif