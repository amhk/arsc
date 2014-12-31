#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"

int map_file(const char *path, void **map, size_t *size)
{
	int fd;
	struct stat st;

	if (stat(path, &st) < 0)
		die("stat");
	fd = open(path, O_RDONLY);
	if (fd < 0)
		die("open");
	*map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (*map == MAP_FAILED)
		die("mmap");
	*size = st.st_size;

	return fd;
}

void unmap_file(int fd, void *map, size_t size)
{
	munmap(map, size);
	close(fd);
}

void __die(const char *file, unsigned int line, const char *func,
	   const char *fmt, ...)
{
	va_list ap;
	int saved_errno = errno;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d:%s: ", file, line, func);
	vfprintf(stderr, fmt, ap);
	if (saved_errno)
		fprintf(stderr, ": %d %s", errno, strerror(errno));
	fprintf(stderr, "\n");
	va_end(ap);

#if 0
	abort();
#else
	exit(1);
#endif
}
