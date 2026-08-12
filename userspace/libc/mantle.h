#ifndef MANTLE_USER_LIBC_H
#define MANTLE_USER_LIBC_H

typedef unsigned long long uint64_t;
typedef long long int64_t;

int64_t mantle_read(uint64_t fd, void *buffer, uint64_t length);
int64_t mantle_write(uint64_t fd, const void *buffer, uint64_t length);
int64_t mantle_exec(const char *path);
int64_t mantle_chdir(const char *path);
int64_t mantle_getcwd(char *buffer, uint64_t length);
int64_t mantle_getpid(void);
int64_t mantle_readdir(const char *path, char *buffer, uint64_t length);
int64_t mantle_uname(char *buffer, uint64_t length);
void mantle_exit(int64_t status);

uint64_t mantle_strlen(const char *text);
int mantle_streq(const char *left, const char *right);

#endif
