#ifndef UNISTD_H
#define UNISTD_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <io.h>
#include <process.h>
#include <direct.h>
#include <sys/stat.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SSIZE_T_DEFINED
#ifndef _SSIZE_T_
typedef intptr_t ssize_t;
#define _SSIZE_T_
#define _SSIZE_T_DEFINED
#endif
#endif

#ifndef _MODE_T_
typedef unsigned short mode_t;
#define _MODE_T_
#endif

#ifndef _UID_T_
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#define _UID_T_
#endif

#ifndef _USECONDS_T_
typedef unsigned int useconds_t;
#define _USECONDS_T_
#endif

#ifndef _PID_T_DECLARED
#ifndef _PID_T_
typedef int pid_t;
#define _PID_T_
#define _PID_T_DECLARED
#endif
#endif

#if !defined(_MSC_VER)
int     open(const char *pathname, int flags, ...);
int     close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t   lseek(int fd, off_t offset, int whence);
int     dup(int oldfd);
int     dup2(int oldfd, int newfd);
int     pipe(int pipefd[2]);
int     isatty(int fd);
int     unlink(const char *pathname);
int     rmdir(const char *pathname);
int     mkdir(const char *pathname, mode_t mode);
char   *getcwd(char *buf, size_t size);
int     chdir(const char *path);
int     access(const char *pathname, int mode);
int     chmod(const char *pathname, mode_t mode);
int     fchmod(int fd, mode_t mode);
mode_t  umask(mode_t mask);
int     truncate(const char *path, off_t length);
int     ftruncate(int fd, off_t length);

int     mkstemp(char *template_name);
int     fork(void);
void    _exit(int status);
int     execv(const char *path, char *const argv[]);
int     execvp(const char *file, char *const argv[]);
int     execve(const char *path, char *const argv[], char *const envp[]);
int     spawnv(int mode, const char *path, char *const argv[]);
int     spawnvp(int mode, const char *file, char *const argv[]);

pid_t   getpid(void);
pid_t   getppid(void);
uid_t   getuid(void);
gid_t   getgid(void);
uid_t   geteuid(void);
gid_t   getegid(void);

unsigned int sleep(unsigned int seconds);
int     usleep(useconds_t usec);
#else
int     mkstemp(char *template_name);
int     fork(void);
#endif

char   *dirname(char *path);
char   *basename(char *path);
char   *ctime_r(const time_t *timer, char *buf);
int     strncasecmp(const char *a, const char *b, size_t n);
char   *strndup(const char *s, size_t n);
char   *_fullpath(char *absPath, const char *relPath, size_t maxLength);
char   *realpath(const char *path, char *resolved_path);

#ifdef __cplusplus
}
#endif

#endif /* UNISTD_H */
