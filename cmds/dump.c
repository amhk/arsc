#include "blob.h"
#include "common.h"
#include "filemap.h"
#include "options.h"

static struct option_spec dump_option_specs[] = {
	OPT_END,
};

int cmd_dump(int argc, char **argv)
{
	struct mapped_file map;
	struct blob *blob;

	argc = parse_options(dump_option_specs, argc, argv);

	die_if(argc == 0, "usage: arsc dump <resource-file-or-apk>");

	map_file(argv[0], &map);
	blob_init(&blob, map.data, map.data_size);
	blob_dump(blob);
	blob_destroy(blob);
	unmap_file(&map);

	return 0;
}
