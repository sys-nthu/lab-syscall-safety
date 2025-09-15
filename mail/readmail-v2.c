// Build:  gcc -Wall -Wextra -O2 readmail-v2.c mail-common.c -o readmail-v2
// Usage (run as root):  ./readmail-v2 <username> [mailfile]
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mail-common.h"

static int drop_to_user(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (!pw) { fprintf(stderr, "unknown user: %s\n", username); return -1; }

    if (initgroups(pw->pw_name, pw->pw_gid) == -1) {
        perror("initgroups"); return -1;
    }
    if (setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) == -1) {
        perror("setresgid"); return -1;
    }
    if (setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) == -1) {
        perror("setresuid"); return -1;
    }

    if (geteuid() == 0 || getuid() == 0) {
        fprintf(stderr, "failed to drop privileges\n");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <username> [mailfile]\n", argv[0]);
        return 2;
    }
    if (geteuid() != 0) {
        fprintf(stderr, "must run as root to change uid\n");
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

    if (drop_to_user(username) == -1) return 1;

    char path[256];
    snprintf(path, sizeof(path), "mails/%s", username);

    if (mailfile) {
        char mailpath[512];
        snprintf(mailpath, sizeof(mailpath), "%s/%s", path, mailfile);
        int fd = open(mailpath, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        print_mail_from_fd(fd);
        close(fd);
    } else {
        DIR *d = opendir(path);
        if (d == NULL) {
            perror("opendir");
            return 1;
        }
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            printf("--- Mail: %s ---\n", ent->d_name);
            char mailpath[512];
            snprintf(mailpath, sizeof(mailpath), "%s/%s", path, ent->d_name);
            int fd = open(mailpath, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
            if (fd != -1) {
                print_mail_from_fd(fd);
                close(fd);
            } else {
                perror("open");
            }
        }
        closedir(d);
    }

    return 0;
}
