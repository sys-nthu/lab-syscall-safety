#ifndef MAIL_COMMON_H
#define MAIL_COMMON_H

#include <stddef.h>

int valid_username(const char *u);
int read_password(char *buf, size_t buflen);
int authenticate(const char *username, const char *password);
void print_mail_from_fd(int fd);

#endif // MAIL_COMMON_H
