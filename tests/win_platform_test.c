#define _DEFAULT_SOURCE 1
#define _BSD_SOURCE 1
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <assert.h>
#include <errno.h>

void *sbrk(ptrdiff_t increment);
int getentropy(void *buffer, size_t length);
int ftruncate(int fd, off_t length);
int truncate(const char *path, off_t length);

int main(void) {
    printf("Testing Win64 platform functions...\n");

    // 1. Memory / sbrk
    void *p1 = sbrk(1024);
    assert(p1 != (void *)-1);
    void *p2 = sbrk(0);
    assert((char *)p2 - (char *)p1 == 1024);
    memset(p1, 0xAA, 1024);

    // 2. Entropy
    uint8_t rand_buf[32];
    memset(rand_buf, 0, sizeof(rand_buf));
    int ret_ent = getentropy(rand_buf, sizeof(rand_buf));
    assert(ret_ent == 0);
    int non_zero = 0;
    for (int i = 0; i < 32; i++) {
        if (rand_buf[i] != 0) non_zero++;
    }
    assert(non_zero > 0);

    // 3. Process & System ID
    pid_t pid = getpid();
    assert(pid > 0);
    pid_t ppid = getppid();
    assert(ppid > 0);
    assert(getuid() == 0);
    assert(getgid() == 0);
    assert(geteuid() == 0);
    assert(getegid() == 0);

    // 4. Time
    struct timeval tv;
    int ret_tv = gettimeofday(&tv, NULL);
    assert(ret_tv == 0);
    assert(tv.tv_sec > 1000000);

    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
    int ret_ns = nanosleep(&ts, NULL);
    assert(ret_ns == 0);

    struct tms tms_buf;
    clock_t clk = times(&tms_buf);
    assert(clk >= 0);

    // 5. File I/O
    const char *fname = "test_plat_tmp.txt";
    unlink(fname);

    int fd = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0666);
    assert(fd >= 0);

    const char msg[] = "Hello Platform Win64\n";
    ssize_t written = write(fd, msg, strlen(msg));
    assert(written == (ssize_t)strlen(msg));

    off_t off = lseek(fd, 0, SEEK_SET);
    assert(off == 0);

    char read_buf[64];
    memset(read_buf, 0, sizeof(read_buf));
    ssize_t read_bytes = read(fd, read_buf, sizeof(read_buf));
    assert(read_bytes == written);
    assert(strcmp(read_buf, msg) == 0);

    // 6. Stat & fstat
    struct stat st;
    int ret_fstat = fstat(fd, &st);
    assert(ret_fstat == 0);
    assert(st.st_size == (off_t)written);
    assert(S_ISREG(st.st_mode));

    // 7. ftruncate
    int ret_trunc = ftruncate(fd, 5);
    assert(ret_trunc == 0);
    ret_fstat = fstat(fd, &st);
    assert(ret_fstat == 0);
    assert(st.st_size == 5);

    // 8. dup / dup2
    int fd_dup = dup(fd);
    assert(fd_dup >= 0);
    int fd_dup2 = dup2(fd, 100);
    assert(fd_dup2 == 100);

    close(fd_dup);
    close(fd_dup2);
    close(fd);

    int ret_stat = stat(fname, &st);
    assert(ret_stat == 0);
    assert(st.st_size == 5);

    // 9. Rename & Remove
    const char *fname2 = "test_plat_tmp2.txt";
    unlink(fname2);
    int ret_ren = rename(fname, fname2);
    assert(ret_ren == 0);
    assert(access(fname2, 0) == 0);
    int ret_rem = remove(fname2);
    assert(ret_rem == 0);
    assert(access(fname2, 0) != 0);

    // 10. Pipe
    int pipefds[2];
    int ret_pipe = pipe(pipefds);
    assert(ret_pipe == 0);
    assert(pipefds[0] >= 0);
    assert(pipefds[1] >= 0);

    const char pmsg[] = "pipedata";
    written = write(pipefds[1], pmsg, 8);
    assert(written == 8);
    char pbuf[16];
    memset(pbuf, 0, sizeof(pbuf));
    read_bytes = read(pipefds[0], pbuf, 8);
    assert(read_bytes == 8);
    assert(memcmp(pbuf, pmsg, 8) == 0);
    close(pipefds[0]);
    close(pipefds[1]);

    // 11. Directory
    const char *testdir = "test_plat_dir";
    rmdir(testdir);
    int ret_mkdir = mkdir(testdir, 0777);
    assert(ret_mkdir == 0);
    int ret_rmdir = rmdir(testdir);
    assert(ret_rmdir == 0);

    // 12. Getcwd
    char cwd[1024];
    char *ret_cwd = getcwd(cwd, sizeof(cwd));
    assert(ret_cwd != NULL);
    assert(strlen(cwd) > 0);

    // 13. isatty
    int is_stdin_tty = isatty(0);
    (void)is_stdin_tty;

    printf("All Win64 platform tests passed successfully!\n");
    return 0;
}
