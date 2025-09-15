// Usage: ./create-base <target-file>
// Intentionally does NOT check errors.
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "demo-created.txt";
    int fd = creat(path, 0644);   // truncates if exists; creates if not
    // no error checking, no close error check
    write(fd, "hello\n", 6);
    close(fd);
    return 0;
}
