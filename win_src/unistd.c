#include <string.h>
#include <time.h>

char *dirname(char *path) {
    char *p = strrchr(path, '/');
    char *q = strrchr(path, '\\');

    if (q && (!p || q > p))
        p = q;

    if (!p)
        return ".";

    // Remove trailing separators.
    while (p > path && (p[-1] == '/' || p[-1] == '\\'))
        p--;

    if (p == path)
        return path[0] ? path : ".";

    *p = '\0';
    return path;
}

char *basename(char *path) {
    char *p = strrchr(path, '/');
    char *q = strrchr(path, '\\');

    if (q && (!p || q > p))
        p = q;

    return p ? p + 1 : path;
}

char *ctime_r(const time_t *timer, char *buf) {
    if (ctime_s(buf, 26, timer) != 0)
        return NULL;
    return buf;
}

int strncasecmp(const char *a, const char *b, size_t n) {
    return _strnicmp(a, b, n);
}