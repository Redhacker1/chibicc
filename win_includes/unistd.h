#ifndef UNISTD_H
#define UNISTD_H
char *dirname(char *path);
char *basename(char *path);
char *ctime_r(const time_t *timer, char *buf);
int strncasecmp(const char *a, const char *b, size_t n);
#endif