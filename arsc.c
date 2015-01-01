#include <stdio.h>
#include <stdlib.h>

#include "blob.h"
#include "common.h"

int main(int argc, char **argv)
{
	int fd = 0;
	void *map;
	size_t size;
	struct blob *blob;

	if (argc < 2) {
		fprintf(stderr, "usage: arsc path-to-resource.arsc\n");
		exit(1);
	}

	fd = map_file(argv[1], &map, &size);
	blob_init(&blob, map, size);
	blob_dump(blob);
	blob_destroy(blob);
	unmap_file(fd, map, size);

	return 0;
}
