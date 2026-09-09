#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glob.h"


static char *glob_strdup(const char *str)
{
    size_t len = strlen(str);
    char *copy = malloc(len + 1);

    if (!copy)
        return NULL;

    memcpy(copy, str, len + 1);
    return copy;
}


static int append_path(glob_t *pglob, const char *path)
{
    size_t count = pglob->gl_pathc;

    char **new_paths = realloc(
        pglob->gl_pathv,
        sizeof(char *) * (count + 2)
    );

    if (!new_paths)
        return -1;

    pglob->gl_pathv = new_paths;

    pglob->gl_pathv[count] = glob_strdup(path);

    if (!pglob->gl_pathv[count])
        return -1;

    pglob->gl_pathc = count + 1;
    pglob->gl_pathv[count + 1] = NULL;

    return 0;
}


int glob(const char *pattern,
         int flags,
         int (*errfunc)(const char *, int),
         glob_t *pglob)
{
    WIN32_FIND_DATAA data;
    HANDLE handle;
    char directory[MAX_PATH];
    char search[MAX_PATH];

    (void)errfunc;

    if (!pglob || !pattern)
        return -1;

    /*
     * GLOB_APPEND is not currently needed by chibicc.
     * Start from an empty result otherwise.
     */
    if (!(flags & GLOB_APPEND)) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
    }

    /*
     * FindFirstFileA accepts patterns such as:
     *
     *     *.c
     *     src\*.c
     *     foo\*.h
     *
     * We can simply pass the pattern through.
     */
    strncpy(search, pattern, MAX_PATH - 1);
    search[MAX_PATH - 1] = '\0';

    handle = FindFirstFileA(search, &data);

    if (handle == INVALID_HANDLE_VALUE) {
        /*
         * No matches.
         *
         * GLOB_NOCHECK means the pattern itself should be returned.
         */
        if (flags & GLOB_NOCHECK) {
            if (append_path(pglob, pattern) != 0) {
                globfree(pglob);
                return -1;
            }

            return 0;
        }

        return 0;
    }

    /*
     * Find the directory portion of the search pattern.
     *
     * Example:
     *
     *     src\*.c
     *
     * becomes:
     *
     *     src\
     */
    {
        const char *slash1 = strrchr(pattern, '\\');
        const char *slash2 = strrchr(pattern, '/');
        const char *slash = slash1 > slash2 ? slash1 : slash2;

        if (slash) {
            size_t len = (size_t)(slash - pattern) + 1;

            if (len >= sizeof(directory)) {
                FindClose(handle);
                globfree(pglob);
                return -1;
            }

            memcpy(directory, pattern, len);
            directory[len] = '\0';
        } else {
            directory[0] = '\0';
        }
    }

    do {
        char path[MAX_PATH];

        /*
         * Skip "." and "..".
         */
        if (strcmp(data.cFileName, ".") == 0 ||
            strcmp(data.cFileName, "..") == 0)
            continue;

        if (directory[0]) {
            snprintf(
                path,
                sizeof(path),
                "%s%s",
                directory,
                data.cFileName
            );
        } else {
            snprintf(
                path,
                sizeof(path),
                "%s",
                data.cFileName
            );
        }

        /*
         * GLOB_ONLYDIR
         */
        if (flags & GLOB_ONLYDIR) {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;
        }

        /*
         * GLOB_MARK
         */
        if (flags & GLOB_MARK) {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                size_t len = strlen(path);

                if (len + 1 < sizeof(path)) {
                    path[len] = '\\';
                    path[len + 1] = '\0';
                }
            }
        }

        if (append_path(pglob, path) != 0) {
            FindClose(handle);
            globfree(pglob);
            return -1;
        }

    } while (FindNextFileA(handle, &data));

    FindClose(handle);

    /*
     * The POSIX glob() normally sorts its results.
     *
     * We can implement that later if needed. For chibicc this
     * generally isn't important because the compiler is given
     * explicit source files.
     */
    return 0;
}


void globfree(glob_t *pglob)
{
    size_t i;

    if (!pglob)
        return;

    for (i = 0; i < pglob->gl_pathc; i++)
        free(pglob->gl_pathv[i]);

    free(pglob->gl_pathv);

    pglob->gl_pathv = NULL;
    pglob->gl_pathc = 0;
}