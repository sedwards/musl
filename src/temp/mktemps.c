#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

/* This assumes that a check for the
   template size has already been made */
char *__randname(char *template)
{
	int i;
	struct timespec ts;
	unsigned long r;

	__clock_gettime(CLOCK_REALTIME, &ts);
	r = ts.tv_nsec*65537 ^ (uintptr_t)&ts / 16 + (uintptr_t)template;
	for (i=0; i<6; i++, r>>=5)
		template[i] = 'A'+(r&15)+(r&16)*2;

	return template;
}

char *mkdtemp(char *template)
{
	size_t l = strlen(template);
	int retries = 100;

	if (l<6 || memcmp(template+l-6, "XXXXXX", 6)) {
		errno = EINVAL;
		return 0;
	}

	do {
		__randname(template+l-6);
		if (!mkdir(template, 0700)) return template;
	} while (--retries && errno == EEXIST);

	memcpy(template+l-6, "XXXXXX", 6);
	return 0;
}
#define _BSD_SOURCE
#include <stdlib.h>

int mkostemp(char *template, int flags)
{
	return __mkostemps(template, 0, flags);
}

weak_alias(mkostemp, mkostemp64);
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int __mkostemps(char *template, int len, int flags)
{
	size_t l = strlen(template);
	if (l<6 || len>l-6 || memcmp(template+l-len-6, "XXXXXX", 6)) {
		errno = EINVAL;
		return -1;
	}

	flags -= flags & O_ACCMODE;
	int fd, retries = 100;
	do {
		__randname(template+l-len-6);
		if ((fd = open(template, flags | O_RDWR | O_CREAT | O_EXCL, 0600))>=0)
			return fd;
	} while (--retries && errno == EEXIST);

	memcpy(template+l-len-6, "XXXXXX", 6);
	return -1;
}

weak_alias(__mkostemps, mkostemps);
weak_alias(__mkostemps, mkostemps64);
#include <stdlib.h>

int mkstemp(char *template)
{
	return __mkostemps(template, 0, 0);
}

weak_alias(mkstemp, mkstemp64);
#define _BSD_SOURCE
#include <stdlib.h>

int mkstemps(char *template, int len)
{
	return __mkostemps(template, len, 0);
}

weak_alias(mkstemps, mkstemps64);
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

char *mktemp(char *template)
{
	size_t l = strlen(template);
	int retries = 100;
	struct stat st;

	if (l < 6 || memcmp(template+l-6, "XXXXXX", 6)) {
		errno = EINVAL;
		*template = 0;
		return template;
	}

	do {
		__randname(template+l-6);
		if (stat(template, &st)) {
			if (errno != ENOENT) *template = 0;
			return template;
		}
	} while (--retries);

	*template = 0;
	errno = EEXIST;
	return template;
}
