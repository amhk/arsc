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
		uint32_t type_id_offset;
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

/*
 * Information about ongoing parsing of resources.arsc blob.
 */
struct parser_context {
	const void *map;
	size_t map_size;
	size_t offset;

	uint32_t next_package;
	enum {
		SP_NONE,
		SP_VALUES,
		SP_TYPE_NAMES,
		SP_RES_NAMES,
	} next_string_pool;
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

static void parse_string_pool(struct parser_context *ctx, struct blob *blob)
{
	die_if(!blob->sp_values && ctx->next_string_pool != SP_VALUES,
	       "offset=%zd: unexpected string pool type %d",
	       ctx->offset, ctx->next_string_pool);

	const struct arsc_string_pool *pool =
		(struct arsc_string_pool *)&ctx->map[ctx->offset];
	struct package *pkg = NULL;

	switch (ctx->next_string_pool) {
	case SP_VALUES:
		blob->sp_values = pool;
		ctx->next_string_pool = SP_NONE;
		break;
	case SP_TYPE_NAMES:
		die_if(ctx->next_package == 0,
		       "offset=%zd: unexpected type name string pool",
		       ctx->offset);
		pkg = &blob->packages[ctx->next_package - 1];
		die_if(pkg->sp_type_names,
		       "offset=%zd: unexpected extra type name string pool",
		       ctx->offset);
		pkg->sp_type_names = pool;
		ctx->next_string_pool = SP_RES_NAMES;
		break;
	case SP_RES_NAMES:
		die_if(ctx->next_package == 0,
		       "offset=%zd: unexpected resource name string pool",
		       ctx->offset);
		pkg = &blob->packages[ctx->next_package - 1];
		die_if(pkg->sp_resource_names,
		       "offset=%zd: unexpected extra resource name string pool",
		       ctx->offset);
		pkg->sp_resource_names = pool;
		ctx->next_string_pool = SP_NONE;
		break;
	case SP_NONE:
		die("offset=%zd: did not expect string pool", ctx->offset);
	}
	ctx->offset += dtohs(pool->header.size);
}

static void parse_blob_header(struct parser_context *ctx, struct blob *blob)
{
	die_if(blob->header, "offset=%zd: extra blob header", ctx->offset);

	blob->header = (struct arsc_header *)&ctx->map[ctx->offset];
	blob->packages = xcalloc(dtohl(blob->header->data.package_count),
				 sizeof(struct package));
	ctx->next_string_pool = SP_VALUES;

	ctx->offset += dtohs(blob->header->header.header_size);
}

static void parse_package(struct parser_context *ctx, struct blob *blob)
{
	die_if(!blob->header,
	       "offset=%zd: package before blob header", ctx->offset);
	die_if(ctx->next_package >= dtohl(blob->header->data.package_count),
	       "offset=%zd: unexpected additional package", ctx->offset);

	const struct arsc_package *a_pkg =
		(struct arsc_package *)&ctx->map[ctx->offset];
	struct package *pkg = &blob->packages[ctx->next_package];
	pkg->package = a_pkg;
	pkg->sp_type_names = NULL;
	pkg->sp_resource_names = NULL;
	pkg->spec_count = 0;
	pkg->max_spec_count = 2;
	pkg->specs = xcalloc(pkg->max_spec_count, sizeof(struct type_spec));

	ctx->next_string_pool = SP_TYPE_NAMES;
	ctx->next_package++;
	ctx->offset += dtohs(a_pkg->header.header_size);
}

static void parse_type(struct parser_context *ctx, struct blob *blob)
{
	die_if(ctx->next_package == 0,
	       "offset=%zd: type found before package", ctx->offset);
	const struct arsc_type *a_type =
		(struct arsc_type *)&ctx->map[ctx->offset];
	struct package *pkg = &blob->packages[ctx->next_package - 1];
	die_if(pkg->spec_count == 0,
	       "offset=%zd: type found before type spec", ctx->offset);
	struct type_spec *spec = &pkg->specs[pkg->spec_count - 1];

	if (spec->type_count == spec->max_type_count) {
		spec->max_type_count *= 2;
		size_t n = spec->max_type_count * sizeof(struct arsc_type *);
		spec->types = xrealloc(spec->types, n);
	}
	spec->types[spec->type_count++] = a_type;

	ctx->offset += dtohs(a_type->header.size);
}

static void parse_type_spec(struct parser_context *ctx, struct blob *blob)
{
	die_if(ctx->next_package == 0,
	       "offset=%zd: type spec found before package", ctx->offset);
	const struct arsc_type_spec *a_spec =
		(struct arsc_type_spec *)&ctx->map[ctx->offset];
	struct package *pkg = &blob->packages[ctx->next_package - 1];

	if (pkg->spec_count == pkg->max_spec_count) {
		pkg->max_spec_count *= 2;
		size_t n = pkg->max_spec_count * sizeof(struct type_spec);
		pkg->specs = xrealloc(pkg->specs, n);
	}
	struct type_spec *spec = &pkg->specs[pkg->spec_count++];
	spec->spec = a_spec;
	spec->type_count = 0;
	spec->max_type_count = 2;
	spec->types = xcalloc(spec->max_type_count, sizeof(struct arsc_type *));

	ctx->offset += dtohs(a_spec->header.size);
}

void blob_init(struct blob **blob_pp, const void *map, size_t map_size)
{
	struct blob *blob = xmalloc(sizeof(*blob));
	blob->header = NULL;
	blob->sp_values = NULL;
	blob->packages = NULL;

	struct parser_context ctx = {
		.map = map,
		.map_size = map_size,
		.offset = 0,
		.next_package = 0,
		.next_string_pool = SP_NONE,
	};

	/* parse resource.arsc blob */
	while (ctx.offset < ctx.map_size) {
		uint16_t type = peek_uint16(ctx.map, ctx.offset);
		switch (type) {
		case 0x0001: /* string pool */
			parse_string_pool(&ctx, blob);
			break;
		case 0x0002: /* blob header */
			parse_blob_header(&ctx, blob);
			break;
		case 0x0200: /* package */
			parse_package(&ctx, blob);
			break;
		case 0x0201: /* type */
			parse_type(&ctx, blob);
			break;
		case 0x0202: /* type spec */
			parse_type_spec(&ctx, blob);
			break;
		default:
			die("unknown type 0x%04x", type);
		}
	}

	/* check invariants */
	die_if(ctx.offset != ctx.map_size,
	       "offset=%zd, blob size=%zd: parsing did not end at end of blob",
	       ctx.offset, ctx.map_size);
	die_if(ctx.next_package != dtohl(blob->header->data.package_count),
	       "package count %d does not match expected package count %d",
	       ctx.next_package, dtohl(blob->header->data.package_count));

	*blob_pp = blob;
}

void blob_destroy(struct blob *blob)
{
	uint32_t i;

	for (i = 0; i < dtohl(blob->header->data.package_count); i++) {
		const struct package *pkg = &blob->packages[i];
		size_t j;

		for (j = 0; j < pkg->spec_count; j++) {
			const struct type_spec *spec = &pkg->specs[j];

			free(spec->types);
		}
		free(pkg->specs);
	}
	free(blob->packages);
	free(blob);
}

void blob_dump(const struct blob *blob)
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
