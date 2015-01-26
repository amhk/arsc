#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "filemap.h"

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
