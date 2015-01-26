#ifndef ARSC_FILEMAP_H
#define ARSC_FILEMAP_H

/*
 * Struct representing a mmap'ed file.
 *
 * Clients should only use the data and data_size fields. The other
 * fields are used for internal book-keeping. (For plain resources.arsc
 * files, map and data are the same. For zip files, map represents the
 * entire zip file and data the resources.arsc file stored within the
 * zip.)
 */
struct mapped_file {
	const void *map;
	size_t map_size;
	int fd;

	const void *data;
	size_t data_size;
};

/*
 * Memory map a file. Accepts both plain resources.arsc files and apk files.
 */
void map_file(const char *path, struct mapped_file *map);
void unmap_file(const struct mapped_file *map);

#endif
