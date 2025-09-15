// Usage: ./create-v1 <target-file>
// Adds simple error reporting with perror.
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <target-file>\n", argv[0]);
        return 2;
    }
    const char *path = argv[1];
    int fd = creat(path, 0644);
    if (fd == -1) { perror("creat"); return errno; }

    ssize_t w = write(fd, "hello\n", 6);
    if (w == -1) { perror("write"); close(fd); return errno; }

    if (close(fd) == -1) { perror("close"); return errno; }
    return 0;
}
