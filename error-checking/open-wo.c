// Usage: ./open-wo <target-file>
// Opens write-only and appends a demo line, reporting errors with perror.
// Great for demonstrating EACCES, ENOENT, EISDIR behaviors.
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <target-file>\n", argv[0]);
        return 2;
    }
    const char *path = argv[1];

    // Try write-only without creating (useful to trigger ENOENT/EACCES)
    int fd = open(path, O_WRONLY);
    if (fd == -1) { perror("open(O_WRONLY)"); return errno; }

    const char *line = "appended by open-wo\n";
    ssize_t w = write(fd, line, 21);
    if (w == -1) { perror("write"); close(fd); return errno; }

    if (close(fd) == -1) { perror("close"); return errno; }
    return 0;
}
