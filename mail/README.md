# Trust-No-Input Mail Reader (Syscall Demo)

This demo shows why you must **check inputs explicitly**—even for file paths—by safely reading `./mails/$USER/mail` after password auth.

## Dev Container
Open this folder in a Microsoft Dev Container (the repo includes `.devcontainer/devcontainer.json`). The image already has `build-essential`.

## Setup (inside the container)
```bash
sudo bash setup.sh
make build
````

## Run

```bash
sudo ./readmail user1
Password: pass1
```

Expected: prints the mail body for `user1`.

Try a wrong password:

```bash
sudo ./readmail user1
Password: nope
# -> authentication failed
```


readmail-v2.c

```c
// Build:  gcc -Wall -Wextra -O2 readmail-v2.c -o readmail-v2
// Usage (run as root):  ./readmail-v2 <username>
#define _GNU_SOURCE
#include <ctype.h>
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

static int valid_username(const char *u) {
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
    fprintf(stdout, "Password: ");
    fflush(stdout);
    if (!fgets(buf, (int)buflen, stdin)) return -1;
    size_t n = strlen(buf);
    if (n && buf[n-1] == '\n') buf[n-1] = '\0';
    return 0;
}

static int authenticate(const char *username, const char *password) {
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
            ok = 1; break;
        }
    }
    fclose(fp);
    return ok ? 0 : 1;
}

// safe open using dir-fd walk + O_NOFOLLOW + S_ISREG check
static int open_mail_safely(const char *username) {
    int mails_fd = open("mails", O_PATH | O_DIRECTORY);
    if (mails_fd == -1) { perror("open mails"); return -1; }

    int userdir_fd = openat(mails_fd, username, O_PATH | O_DIRECTORY | O_NOFOLLOW);
    if (userdir_fd == -1) {
        perror("openat user directory");
        close(mails_fd);
        return -1;
    }

    int fd = openat(userdir_fd, "mail", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd == -1) {
        perror("openat mail");
        close(userdir_fd); close(mails_fd);
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd); close(userdir_fd); close(mails_fd);
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "refused: mail is not a regular file\n");
        close(fd); close(userdir_fd); close(mails_fd);
        return -1;
    }

    close(userdir_fd);
    close(mails_fd);
    return fd;
}

static int drop_to_user(const char *username) {
    // Look up uid/gid for the target user
    struct passwd *pw = getpwnam(username);
    if (!pw) { fprintf(stderr, "unknown user: %s\n", username); return -1; }

    // Set supplementary groups first
    if (initgroups(pw->pw_name, pw->pw_gid) == -1) {
        perror("initgroups"); return -1;
    }
    // Then set primary gid
    if (setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) == -1) {
        perror("setresgid"); return -1;
    }
    // Finally drop uid (real, effective, saved)
    if (setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) == -1) {
        perror("setresuid"); return -1;
    }

    // Sanity check: we should not be root anymore
    if (geteuid() == 0 || getuid() == 0) {
        fprintf(stderr, "failed to drop privileges\n");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <username>\n", argv[0]);
        return 2;
    }
    if (geteuid() != 0) {
        fprintf(stderr, "must run as root to change uid\n");
        return 2;
    }

    const char *username = argv[1];
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

    // Drop privileges to the target user BEFORE opening the mail file.
    if (drop_to_user(username) == -1) return 1;

    // Now open as the target user. Permissions on mails/$user/mail (0600 owned by user)
    // will be enforced by the kernel.
    int fd = open_mail_safely(username);
    if (fd == -1) return 1;

    // Stream file to stdout
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(STDOUT_FILENO, buf + off, (size_t)(n - off));
            if (w == -1) { perror("write"); close(fd); return 1; }
            off += w;
        }
    }
    if (n == -1) perror("read");

    close(fd);
    return (n == -1) ? 1 : 0;
}
```

## Safety checks you can point out in class

* **Username validation**: `[a-z0-9_]`, length ≤ 32 (prevents `../`).
* **Directory-fd walk**: `open("mails", O_PATH|O_DIRECTORY)` → `openat(userdir, ...) `.
* **O_NOFOLLOW**: refuses symlink tricks on both the user directory and the `mail` file.
* **Type check**: `fstat` + `S_ISREG` rejects FIFOs, sockets, devices, etc.

## A couple of attack demos (and what happens)

```bash
# 1) Symlink the mail file (program should refuse)
sudo mv mails/user1/mail mails/user1/mail.bak
sudo ln -s /etc/passwd mails/user1/mail
sudo ./readmail user1
Password: pass1
# -> openat mail: ... (refused due to O_NOFOLLOW)

# 2) Replace with a FIFO (program should refuse)
sudo rm -f mails/user2/mail
sudo mkfifo mails/user2/mail
sudo ./readmail user2
Password: pass2
# -> refused: mail is not a regular file
```

## Clean up

```bash
sudo rm -rf mails passwords.txt
sudo userdel user1 || true
sudo userdel user2 || true
sudo userdel user3 || true
```

> Educational note: In production, don’t store plaintext passwords; use proper hashing (e.g., bcrypt/argon2) or PAM. This is a **teaching** demo focused on syscall-level path safety.
