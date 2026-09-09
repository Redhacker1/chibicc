#ifndef CHIBICC_WIN32_SYS_WAIT_H
#define CHIBICC_WIN32_SYS_WAIT_H

#ifdef _WIN32

//#include <windows.h>

/*
 * Minimal POSIX waitpid compatibility for chibicc.
 *
 * chibicc only needs to wait for a child process and inspect
 * whether it exited normally and what its exit status was.
 */

#define WIFEXITED(status)   ((status) != STILL_ACTIVE)
#define WEXITSTATUS(status) ((status) & 0xff)

int waitpid(int pid, int *status, int options);

#endif /* _WIN32 */

#endif /* CHIBICC_WIN32_SYS_WAIT_H */