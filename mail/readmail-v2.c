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
    int previous_uid = geteuid();
    struct passwd *pw = getpwnam(username);
    if (!pw) { perror("getpwnam"); return -1; }

    if (seteuid(pw->pw_uid) == -1) { perror("seteuid"); return -1; }

    printf("previous effective uid=%d, current effective uid=%d\n", previous_uid, geteuid());
    if (geteuid() == 0) { fprintf(stderr, "still root euid\n"); return -1; }
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
