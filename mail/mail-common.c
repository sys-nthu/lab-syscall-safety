#include "mail-common.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int valid_username(const char *u) {
    // allow [a-z0-9_], length 1..32
    size_t n = strlen(u);
    if (n == 0 || n > 32) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)u[i];
        if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
            return 0;
    }
    return 1;
}

int read_password(char *buf, size_t buflen) {
    // For simplicity in class, we leave echo on.
    // (You can switch to termios to hide input if desired.)
    fprintf(stdout, "Password: ");
    fflush(stdout);
    if (!fgets(buf, (int)buflen, stdin)) return -1;
    size_t n = strlen(buf);
    if (n && buf[n-1] == '\n') buf[n-1] = '\0';
    return 0;
}

int authenticate(const char *username, const char *password) {
    FILE *fp = fopen("passwords.txt", "r");
    if (!fp) { perror("fopen passwords.txt"); return -1; }
    char line[256];
    int ok = 0;
    while (fgets(line, sizeof line, fp)) {
        size_t n = strlen(line);
        if (n && line[n-1] == '\n') line[n-1] = '\0';
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        const char *user = line;
        const char *pass = colon + 1;
        if (strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
            ok = 1;
            break;
        }
    }
    fclose(fp);
    return ok ? 0 : 1;
}

void print_mail_from_fd(int fd) {
    // Stream file to stdout
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(STDOUT_FILENO, buf + off, (size_t)(n - off));
            if (w == -1) { perror("write"); return; }
            off += w;
        }
    }
    if (n == -1) perror("read");
}

