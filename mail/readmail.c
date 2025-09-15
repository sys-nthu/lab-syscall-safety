// Build:  gcc -Wall -Wextra -O2 readmail.c mail-common.c -o readmail
// Usage (run as root):  ./readmail <username> [mailfile]
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


static int valid_username(const char *u) {
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

static int read_password(char *buf, size_t buflen) {
    // For simplicity in class, we leave echo on.
    // (You can switch to termios to hide input if desired.)
    fprintf(stdout, "Password: ");
    fflush(stdout);
    if (!fgets(buf, (int)buflen, stdin)) return -1;
    size_t n = strlen(buf);
    if (n && buf[n-1] == '\n') buf[n-1] = '\0';
    return 0;
}

static int authenticate(const char *username, const char *password) {
    FILE *fp = fopen("/tmp/mails/passwords.txt", "r");
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

static void print_mail_from_fd(int fd) {
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

static int open_and_print(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "%s\n", path);
        perror("open");
        return -1;
    }
    print_mail_from_fd(fd);
    close(fd);
    return 0;
}

#ifdef CHANGE_USER
static int run_as_user(const char *username) {
    int previous_uid = geteuid();
    struct passwd *pw = getpwnam(username);
    if (!pw) { perror("getpwnam"); return -1; }

    if (seteuid(pw->pw_uid) == -1) { perror("seteuid"); return -1; }

    printf("previous effective uid=%d, current effective uid=%d\n", previous_uid, geteuid());
    if (geteuid() == 0) { fprintf(stderr, "still root euid\n"); return -1; }
    return 0;
}
#endif

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <username> [mailfile]\n", argv[0]);
        return 2;
    }

    const char *username = argv[1];
    const char *mailfile = (argc == 3) ? argv[2] : NULL;

    if (!valid_username(username)) {
        fprintf(stderr, "invalid username format; allowed [a-z0-9_], len<=32\n");
        return 2;
    }

    char pwbuf[128];
    if (read_password(pwbuf, sizeof pwbuf) == -1) {
        fprintf(stderr, "failed to read password\n");
        return 1;
    }

    int auth = authenticate(username, pwbuf);
    if (auth == -1) return 1;
    if (auth == 1) {
        fprintf(stderr, "authentication failed\n");
        return 1;
    }

#ifdef CHANGE_USER
    if (run_as_user(username) == -1) return 1;
#endif

    char path[PATH_MAX];
    if (mailfile) {
        // open mails/<username>/<mailfile>
        int n = snprintf(path, sizeof path, "/tmp/mails/%s/%s", username, mailfile);
        if (n < 0 || (size_t)n >= sizeof path) {
            fprintf(stderr, "path too long\n");
            return 1;
        }
        return open_and_print(path) == 0 ? 0 : 1;
    }

    // No specific file: list and print all mails in mails/<username>
    int n = snprintf(path, sizeof path, "/tmp/mails/%s", username);
    if (n < 0 || (size_t)n >= sizeof path) {
        fprintf(stderr, "path too long\n");
        return 1;
    }

    DIR *d = opendir(path);
    if (!d) {
        perror("opendir user dir");
        return 1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        char file_path[PATH_MAX];
        int m = snprintf(file_path, sizeof file_path, "%s/%s", path, ent->d_name);
        if (m < 0 || (size_t)m >= sizeof file_path) {
            fprintf(stderr, "path too long for entry %s\n", ent->d_name);
            continue;
        }

        printf("--- Mail: %s ---\n", ent->d_name);
        open_and_print(file_path);
    }
    closedir(d);
    return 0;
}
