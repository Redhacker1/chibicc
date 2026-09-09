#ifndef CHIBICC_WIN32_SYS_WAIT_H
#define CHIBICC_WIN32_SYS_WAIT_H

#ifdef _WIN32

#include <windows.h>
#include <sys/wait.h>

/*
 * Minimal POSIX waitpid compatibility for chibicc.
 *
 * chibicc only needs to wait for a child process and inspect
 * whether it exited normally and what its exit status was.
 */

#define WIFEXITED(status)   ((status) != STILL_ACTIVE)
#define WEXITSTATUS(status) ((status) & 0xff)

int waitpid(
    int pid,
    int *status,
    int options)
{
    HANDLE process;
    DWORD result;

    (void)options;

    process = OpenProcess(
        SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
        FALSE,
        (DWORD)pid
    );

    if (!process)
        return -1;

    result = WaitForSingleObject(process, INFINITE);

    if (result != WAIT_OBJECT_0) {
        CloseHandle(process);
        return -1;
    }

    if (status) {
        DWORD exit_code = 0;

        if (GetExitCodeProcess(process, &exit_code))
            *status = (int)exit_code;
        else
            *status = -1;
    }

    CloseHandle(process);

    return pid;
}

#endif /* _WIN32 */

#endif /* CHIBICC_WIN32_SYS_WAIT_H */