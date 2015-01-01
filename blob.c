#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "blob.h"
#include "common.h"

/*
 * arsc_* structs. Read-only pointers to the resource.arsc blob copied from
 * Android's ResourceTypes.h. All integer access must be wrapped in dtohl or
 * dtohs.
 */
struct arsc_chunk_header {
	uint16_t type;
	uint16_t header_size;
	uint32_t size;
};

struct arsc_header {
	struct arsc_chunk_header header;
	struct {
		uint32_t package_count;
	} data;
};

struct arsc_string_pool {
	struct arsc_chunk_header header;
	struct {
		uint32_t string_count;
		uint32_t style_count;
		uint32_t flags;
		uint32_t strings_start;
		uint32_t styles_start;
	} data;
};

struct arsc_package {
	struct arsc_chunk_header header;
	struct {
		uint32_t id;
		uint16_t name[128];
		uint32_t type_strings;
		uint32_t last_public_type;
		uint32_t key_strings;
		uint32_t last_public_key;
		uint32_t type_id_offset; /* FIXME: include this? */
	} data;
};

struct arsc_type_spec {
	struct arsc_chunk_header header;
	struct {
		uint8_t id;
		uint8_t res0;
		uint8_t res1;
		uint32_t entry_count;
	} data;
};

struct arsc_config {
	uint32_t size;

	uint16_t mcc;
	uint16_t mnc;

	uint16_t language;
	uint16_t country;

	uint16_t orientation;
	uint16_t touchscreen;
	uint16_t density;

	uint16_t keyboard;
	uint16_t navigation;
	uint16_t input_flags;
	uint16_t input_pad0;

	uint16_t screen_width;
	uint16_t screen_height;

	uint16_t sdk_version;
	uint16_t minor_version;

	uint16_t screen_layout;
	uint16_t ui_mode;
	uint16_t smallest_screen_width_dp;

	uint16_t screen_width_dp;
	uint16_t screen_height_dp;
};

struct arsc_type {
	struct arsc_chunk_header header;
	struct {
		uint8_t id;
		uint8_t res0;
		uint8_t res1;
		uint32_t entry_count;
		uint32_t entries_start;
		struct arsc_config config;
	} data;
};

/*
 * Writeable wrapper structs.
 */
struct type_spec {
	const struct arsc_type_spec *spec;
	const struct arsc_type **types;
	size_t type_count;
	size_t max_type_count;
};

struct package {
	const struct arsc_package *package;
	const struct arsc_string_pool *sp_type_names;
	const struct arsc_string_pool *sp_resource_names;
	struct type_spec *specs;
	size_t spec_count;
	size_t max_spec_count;
};

struct blob {
	const struct arsc_header *header;
	const struct arsc_string_pool *sp_values;
	struct package *packages;
};

#define check_alignment(offset, alignment) \
	do { \
		if (offset % alignment != 0) \
			die("offset not on %d byte alignment", alignment); \
	} while (0)

static inline uint16_t peek_uint16(const uint8_t *map, size_t offset)
{
	const uint16_t *p;
	check_alignment(offset, 2);
	p = (const uint16_t *)(map + offset);
	return dtohs(*p);
}

void blob_init(struct blob **blob_pp, const void *map, size_t size)
{
	size_t offset = 0;
	struct blob *blob = xmalloc(sizeof(*blob));
	blob->header = NULL;
	blob->sp_values = NULL;
	blob->packages = NULL;

	int current_package = -1;
	enum {
		NONE,
		VALUES,
		TYPE_NAMES,
		RES_NAMES,
	} next_string_pool = NONE;

	/* parse resource.arsc blob */
	while (offset < size) {
		uint16_t type = peek_uint16(map, offset);
		switch (type) {
		case 0x0001: /* string pool */
			{
				die_if(!blob->sp_values && next_string_pool != VALUES,
				       "unexpected string pool type at offset %zd", offset);

				const struct arsc_string_pool *pool = (struct arsc_string_pool *)&map[offset];
				switch (next_string_pool) {
				case VALUES:
					blob->sp_values = pool;
					next_string_pool = NONE;
					break;
				case TYPE_NAMES:
					die_if(current_package < 0, "type name string pool found before package at offset %zd", offset);
					{
						struct package *pkg = &blob->packages[current_package];
						die_if(pkg->sp_type_names, "extra type name string pool found at offset %zd", offset);
						pkg->sp_type_names = pool;
					}
					next_string_pool = RES_NAMES;
					break;
				case RES_NAMES:
					die_if(current_package < 0, "resource name string pool fround before package at offset %zd", offset);
					{
						struct package *pkg = &blob->packages[current_package];
						die_if(pkg->sp_resource_names, "extra resource name string pool found at offset %zd", offset);
						pkg->sp_resource_names = pool;
					}
					next_string_pool = NONE;
					break;
				case NONE:
					die("did not expect string pool at offset %zd\n", offset);
				}
				offset += dtohs(pool->header.size);
			}
			break;
		case 0x0002: /* blob header */
			die_if(blob->header, "extra blob header at offset %zd", offset);

			blob->header = (struct arsc_header *)&map[offset];
			blob->packages = xcalloc(dtohl(blob->header->data.package_count), sizeof(struct package));
			next_string_pool = VALUES;

			offset += dtohs(blob->header->header.header_size);
			break;
		case 0x0200: /* package */
			{
				current_package++;
				die_if(!blob->header, "found package before blob header at offset %zd", offset);
				die_if((uint32_t)current_package > dtohl(blob->header->data.package_count), "found additional package after expected package count %zd", dtohl(blob->header->data.package_count));

				const struct arsc_package *a_pkg = (struct arsc_package *)&map[offset];
				struct package *pkg = &blob->packages[current_package];
				pkg->package = a_pkg;
				pkg->sp_type_names = NULL;
				pkg->sp_resource_names = NULL;
				pkg->spec_count = 0;
				pkg->max_spec_count = 2;
				pkg->specs = xcalloc(pkg->max_spec_count, sizeof(struct type_spec));

				next_string_pool = TYPE_NAMES;
				offset += dtohs(a_pkg->header.header_size);
			}
			break;
		case 0x0201: /* type */
			{
				die_if(current_package < 0, "type found before package at offset %zd", offset);
				const struct arsc_type *a_type = (struct arsc_type *)&map[offset];
				struct package *pkg = &blob->packages[current_package];
				die_if(pkg->spec_count == 0, "type found before type spec at offset %zd", offset);
				struct type_spec *spec = &pkg->specs[pkg->spec_count - 1];

				if (spec->type_count == spec->max_type_count) {
					spec->max_type_count *= 2;
					spec->types = xrealloc(spec->types, spec->max_type_count * sizeof(struct arsc_type *));
				}
				spec->types[spec->type_count++] = a_type;

				offset += dtohs(a_type->header.size);
			}
			break;
		case 0x0202: /* type spec */
			{
				die_if(current_package < 0, "type spec found before package at offset %zd", offset);
				const struct arsc_type_spec *a_spec = (struct arsc_type_spec *)&map[offset];
				struct package *pkg = &blob->packages[current_package];

				if (pkg->spec_count == pkg->max_spec_count) {
					pkg->max_spec_count *= 2;
					pkg->specs = xrealloc(pkg->specs, pkg->max_spec_count * sizeof(struct type_spec));
				}
				struct type_spec *spec = &pkg->specs[pkg->spec_count++];
				spec->spec = a_spec;
				spec->type_count = 0;
				spec->max_type_count = 2;
				spec->types = xcalloc(spec->max_type_count, sizeof(struct arsc_type *));

				offset += dtohs(a_spec->header.size);
			}
			break;
		default:
			die("unknown type 0x%04x", type);
		}
	}

	/* check invariants */
	die_if(offset != size,
	       "end-of-blob offset %zd does not match blob size %zd",
	       offset, size);
	die_if((uint32_t)current_package + 1 != dtohl(blob->header->data.package_count),
	       "end-of-blob package count %d does not match expected package count %d",
	       current_package + 1, dtohl(blob->header->data.package_count));

	*blob_pp = blob;
}

void blob_dump(const struct blob *blob)
{
	uint32_t i;

	printf("header: package_count=%d\n", dtohl(blob->header->data.package_count));
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

				printf("type: id=0x%02x\n", dtohs(type->data.id));
			}
		}
	}
}
