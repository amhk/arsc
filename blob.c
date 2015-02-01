#include "arsc.h"
#include "blob.h"
#include "common.h"

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
