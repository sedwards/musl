
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include "__dirent.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include "syscall.h"
#include "lock.h"

// string.h GNU_SOURCE
int strverscmp (const char *, const char *);


int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcoll((*a)->d_name, (*b)->d_name);
}

weak_alias(alphasort, alphasort64);

int closedir(DIR *dir)
{
	int ret = close(dir->fd);
	free(dir);
	return ret;
}

int dirfd(DIR *d)
{
	return d->fd;
}

DIR *fdopendir(int fd)
{
	DIR *dir;
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return 0;
	}
	if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		return 0;
	}
	if (!(dir = calloc(1, sizeof *dir))) {
		return 0;
	}

	fcntl(fd, F_SETFD, FD_CLOEXEC);
	dir->fd = fd;
	return dir;
}

DIR *opendir(const char *name)
{
	int fd;
	DIR *dir;

	if ((fd = open(name, O_RDONLY|O_DIRECTORY|O_CLOEXEC)) < 0)
		return 0;
	if (!(dir = calloc(1, sizeof *dir))) {
		__syscall(SYS_close, fd);
		return 0;
	}
	dir->fd = fd;
	return dir;
}

typedef char dirstream_buf_alignment_check[1-2*(int)(
	offsetof(struct __dirstream, buf) % sizeof(off_t))];

struct dirent *readdir(DIR *dir)
{
	struct dirent *de;
	
	if (dir->buf_pos >= dir->buf_end) {
		int len = __syscall(SYS_getdents, dir->fd, dir->buf, sizeof dir->buf);
		if (len <= 0) {
			if (len < 0 && len != -ENOENT) errno = -len;
			return 0;
		}
		dir->buf_end = len;
		dir->buf_pos = 0;
	}
	de = (void *)(dir->buf + dir->buf_pos);
	dir->buf_pos += de->d_reclen;
	dir->tell = de->d_off;
	return de;
}

weak_alias(readdir, readdir64);
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include "lock.h"

int readdir_r(DIR *restrict dir, struct dirent *restrict buf, struct dirent **restrict result)
{
	struct dirent *de;
	int errno_save = errno;
	int ret;
	
	LOCK(dir->lock);
	errno = 0;
	de = readdir(dir);
	if ((ret = errno)) {
		UNLOCK(dir->lock);
		return ret;
	}
	errno = errno_save;
	if (de) memcpy(buf, de, de->d_reclen);
	else buf = NULL;

	UNLOCK(dir->lock);
	*result = buf;
	return 0;
}

weak_alias(readdir_r, readdir64_r);
#include <dirent.h>
#include <unistd.h>
#include "lock.h"

void rewinddir(DIR *dir)
{
	LOCK(dir->lock);
	lseek(dir->fd, 0, SEEK_SET);
	dir->buf_pos = dir->buf_end = 0;
	dir->tell = 0;
	UNLOCK(dir->lock);
}
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>

int scandir(const char *path, struct dirent ***res,
	int (*sel)(const struct dirent *),
	int (*cmp)(const struct dirent **, const struct dirent **))
{
	DIR *d = opendir(path);
	struct dirent *de, **names=0, **tmp;
	size_t cnt=0, len=0;
	int old_errno = errno;

	if (!d) return -1;

	while ((errno=0), (de = readdir(d))) {
		if (sel && !sel(de)) continue;
		if (cnt >= len) {
			len = 2*len+1;
			if (len > SIZE_MAX/sizeof *names) break;
			tmp = realloc(names, len * sizeof *names);
			if (!tmp) break;
			names = tmp;
		}
		names[cnt] = malloc(de->d_reclen);
		if (!names[cnt]) break;
		memcpy(names[cnt++], de, de->d_reclen);
	}

	closedir(d);

	if (errno) {
		if (names) while (cnt-->0) free(names[cnt]);
		free(names);
		return -1;
	}
	errno = old_errno;

	if (cmp) qsort(names, cnt, sizeof *names, (int (*)(const void *, const void *))cmp);
	*res = names;
	return cnt;
}

weak_alias(scandir, scandir64);
#include <dirent.h>
#include <unistd.h>
#include "lock.h"

void seekdir(DIR *dir, long off)
{
	LOCK(dir->lock);
	dir->tell = lseek(dir->fd, off, SEEK_SET);
	dir->buf_pos = dir->buf_end = 0;
	UNLOCK(dir->lock);
}
#include <dirent.h>

long telldir(DIR *dir)
{
	return dir->tell;
}
#define _GNU_SOURCE
#include <string.h>

int versionsort(const struct dirent **a, const struct dirent **b)
{
	return strverscmp((*a)->d_name, (*b)->d_name);
}

#undef versionsort64
weak_alias(versionsort, versionsort64);
int strverscmp (const char *, const char *);
