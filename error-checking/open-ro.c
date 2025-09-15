// Usage: ./open-ro <target-file>
// Opens read-only and reports errors via perror; prints file to stdout.
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <target-file>\n", argv[0]);
        return 2;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open(O_RDONLY)"); return errno; }

    char buf[4096];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n == 0) break;           // EOF
        if (n < 0) { perror("read"); close(fd); return errno; }
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(STDOUT_FILENO, buf + off, (size_t)(n - off));
            if (w < 0) { perror("write"); close(fd); return errno; }
            off += w;
        }
    }
    if (close(fd) == -1) { perror("close"); return errno; }
    return 0;
}
