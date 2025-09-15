// Build:  gcc -Wall -Wextra -O2 readmail.c mail-common.c -o readmail
// Usage (run as root):  ./readmail <username> [mailfile]
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mail-common.h"

static int open_one_mail_safely(int userdir_fd, const char *mailfile) {
    // O_NOFOLLOW to reject symlinked mail file
    int fd = openat(userdir_fd, mailfile, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd == -1) {
        perror("openat mail");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "refused: mail is not a regular file\n");
        close(fd);
        return -1;
    }
    return fd;
}

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
    if (auth == -1) return 1;          // error printed
    if (auth == 1) {
        fprintf(stderr, "authentication failed\n");
        return 1;
    }

    int mails_fd = open("mails", O_PATH | O_DIRECTORY);
    if (mails_fd == -1) {
        perror("open mails");
        return 1;
    }

    int userdir_fd = openat(mails_fd, username, O_PATH | O_DIRECTORY | O_NOFOLLOW);
    if (userdir_fd == -1) {
        perror("openat user directory");
        close(mails_fd);
        return 1;
    }
    close(mails_fd);

    if (mailfile) {
        int fd = open_one_mail_safely(userdir_fd, mailfile);
        if (fd == -1) {
            close(userdir_fd);
            return 1;
        }
        print_mail_from_fd(fd);
        close(fd);
    } else {
        // open a real readable dir fd from the O_PATH dir for fdopendir
        int list_fd = openat(userdir_fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (list_fd == -1) {
            perror("openat .");
            close(userdir_fd);
            return 1;
        }
        DIR *d = fdopendir(list_fd);
        if (d == NULL) {
            perror("fdopendir");
            close(list_fd);
            close(userdir_fd);
            return 1;
        }

        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            printf("--- Mail: %s ---\n", ent->d_name);
            int fd = open_one_mail_safely(userdir_fd, ent->d_name);
            if (fd != -1) {
                print_mail_from_fd(fd);
                close(fd);
            }
        }
        closedir(d);
    }

    close(userdir_fd);
    return 0;
}
