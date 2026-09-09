#ifndef CHIBICC_WIN32_GLOB_H
#define CHIBICC_WIN32_GLOB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t gl_pathc;   /* Number of matched paths */
    char **gl_pathv;   /* NULL-terminated array of paths */
} glob_t;

int glob(const char *pattern,
         int flags,
         int (*errfunc)(const char *, int),
         glob_t *pglob);

void globfree(glob_t *pglob);

/* We don't currently need any POSIX flags. */
#define GLOB_ERR       0x01
#define GLOB_MARK      0x02
#define GLOB_NOSORT    0x04
#define GLOB_NOCHECK   0x08
#define GLOB_APPEND     0x10
#define GLOB_DOOFFS     0x20
#define GLOB_NOSYS      0x40
#define GLOB_BRACE      0x80
#define GLOB_NOMAGIC    0x100
#define GLOB_TILDE      0x200
#define GLOB_ONLYDIR    0x400

#ifdef __cplusplus
}
#endif

#endif