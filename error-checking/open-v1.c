// Usage: ./open-base <target-file>
// Opens with default read-only intent (implementation-defined here), no error checks.
// Then reads and writes to stdout without checks (on purpose).
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "demo-open.txt";
    int fd = open(path, O_RDONLY);   // ignore errors
    char buf[1024];
    ssize_t n = read(fd, buf, sizeof buf);
    write(STDOUT_FILENO, buf, (size_t)n);
    close(fd);
    return 0;
}
