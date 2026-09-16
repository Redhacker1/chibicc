#ifndef CHIBICC_WIN32_GLOB_H
#define CHIBICC_WIN32_GLOB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t gl_pathc;   /* Number of matched paths */
    char **gl_pathv;   /* NULL-terminated array of paths */
    int    gl_offs;    /* Slots to reserve at beginning of gl_pathv */
} glob_t;

int glob(const char *pattern,
         int flags,
         int (*errfunc)(const char *, int),
         glob_t *pglob);

void globfree(glob_t *pglob);

#define GLOB_ERR         0x0001
#define GLOB_MARK        0x0002
#define GLOB_NOSORT      0x0004
#define GLOB_DOOFFS      0x0008
#define GLOB_NOCHECK     0x0010
#define GLOB_APPEND      0x0020
#define GLOB_NOESCAPE    0x0040
#define GLOB_PERIOD      0x0080
#define GLOB_MAGCHAR     0x0100
#define GLOB_ALTDIRFUNC  0x0200
#define GLOB_BRACE       0x0400
#define GLOB_NOMAGIC     0x0800
#define GLOB_TILDE       0x1000
#define GLOB_LIMIT       0x2000
#define GLOB_ONLYDIR     0x4000

#define GLOB_NOSPACE     1
#define GLOB_ABORTED     2
#define GLOB_NOMATCH     3
#define GLOB_NOSYS       4

#ifdef __cplusplus
}
#endif

#endif