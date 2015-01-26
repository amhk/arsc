#include <stdio.h>
#include <stdlib.h>

#include "blob.h"
#include "common.h"
#include "filemap.h"

int main(int argc, char **argv)
{
	struct mapped_file map;
	struct blob *blob;

	if (argc < 2) {
		fprintf(stderr, "usage: arsc path-to-resource.arsc\n");
		exit(1);
	}

	map_file(argv[1], &map);
	blob_init(&blob, map.data, map.data_size);
	blob_dump(blob);
	blob_destroy(blob);
	unmap_file(&map);

	return 0;
}
