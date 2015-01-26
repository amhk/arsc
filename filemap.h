#ifndef ARSC_FILEMAP_H
#define ARSC_FILEMAP_H

/*
 * map_file: mmap a file
 */
int map_file(const char *path, void **out_p, size_t *size);
void unmap_file(int fd, void *map, size_t size);

#endif
