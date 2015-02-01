#include <stdio.h>

#include "arsc.h"
#include "blob.h"
#include "common.h"
#include "filemap.h"
#include "options.h"

static void dump(const struct blob *blob)
{
	uint32_t i;

	printf("header: package_count=%d\n",
	       dtohl(blob->header->data.package_count));
	printf("string pool (resource values): string_count=%d\n",
	       dtohl(blob->sp_values->data.string_count));
	for (i = 0; i < dtohl(blob->header->data.package_count); i++) {
		const struct package *pkg = &blob->packages[i];
		size_t j;

		printf("package: id=0x%02x spec_count=%zd\n",
		       dtohl(pkg->package->data.id), pkg->spec_count);
		printf("string pool (type names): string_count=%d\n",
		       dtohl(pkg->sp_type_names->data.string_count));
		printf("string pool (resource names): string_count=%d\n",
		       dtohl(pkg->sp_resource_names->data.string_count));
		for (j = 0; j < pkg->spec_count; j++) {
			const struct type_spec *spec = &pkg->specs[j];
			size_t k;

			printf("type spec: id=0x%02x type_count=%zd\n",
			       dtohs(spec->spec->data.id), spec->type_count);
			for (k = 0; k < spec->type_count; k++) {
				const struct arsc_type *type = spec->types[k];

				printf("type: id=0x%02x\n",
				       dtohs(type->data.id));
			}
		}
	}
}

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
	dump(blob);
	blob_destroy(blob);
	unmap_file(&map);

	return 0;
}
