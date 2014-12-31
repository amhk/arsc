#include <stdint.h>
#include <stdlib.h>

#include <stdio.h> /* FIXME: rm */

#include "blob.h"
#include "common.h"

struct blob {
	const uint8_t *map;
	size_t size;
	size_t offset;
};

struct chunk_header {
	uint16_t type;
	uint16_t header_size;
	uint32_t size;
};

struct blob_header {
	struct chunk_header header;
	struct {
		uint32_t package_count;
	} data;
};

struct string_pool {
	struct chunk_header header;
	struct {
		uint32_t string_count;
		uint32_t style_count;
		uint32_t flags;
		uint32_t strings_start;
		uint32_t styles_start;
	} data;
};

struct package {
	struct chunk_header header;
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

struct type_spec {
	struct chunk_header header;
	struct {
		uint8_t id;
		uint8_t res0;
		uint8_t res1;
		uint32_t entry_count;
	} data;
};

struct config {
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

struct type {
	struct chunk_header header;
	struct {
		uint8_t id;
		uint8_t res0;
		uint8_t res1;
		uint32_t entry_count;
		uint32_t entries_start;
		struct config config;
	} data;
};

#define check_alignment(blob_p, alignment) \
	do { \
		if (blob_p->offset % alignment != 0) \
			die("offset not on %d byte alignment", alignment); \
	} while (0)

static inline uint16_t peek_uint16(const struct blob *blob)
{
	const uint16_t *p;
	check_alignment(blob, 2);
	p = (const uint16_t *)(blob->map + blob->offset);
	return dtohs(*p);
}

static void parse_blob_header(struct blob *blob)
{
	const struct blob_header *blob_header =
		(struct blob_header *)&blob->map[blob->offset];
	blob->offset += blob_header->header.header_size;
	printf("blob_header: type=0x%04x package_count=%d\n",
	       dtohs(blob_header->header.type), dtohl(blob_header->data.package_count));
}

static void parse_string_pool(struct blob *blob)
{
	const struct string_pool *pool =
		(struct string_pool *)&blob->map[blob->offset];
	blob->offset += pool->header.size;
	printf("string_pool: type=0x%04x string_count=%d\n",
	       dtohs(pool->header.type), dtohl(pool->data.string_count));
}

static void parse_package(struct blob *blob)
{
	const struct package *pkg =
		(struct package *)&blob->map[blob->offset];
	blob->offset += pkg->header.header_size;
	printf("package: type=0x%04x id=0x%02x\n",
	       dtohs(pkg->header.type), dtohl(pkg->data.id));
}

static void parse_type_spec(struct blob *blob)
{
	const struct type_spec *spec =
		(struct type_spec *)&blob->map[blob->offset];
	blob->offset += spec->header.size;
	printf("type_spec: type=0x%04x id=0x%02x\n",
	       dtohs(spec->header.type), dtohl(spec->data.id));
}

static void parse_type(struct blob *blob)
{
	const struct type *type =
		(struct type *)&blob->map[blob->offset];
	blob->offset += type->header.size;
	printf("type: type=0x%04x id=0x%02x\n",
	       dtohs(type->header.type), dtohl(type->data.id));
}

void blob_init(struct blob **blob_pp, const void *map, size_t size)
{
	struct blob *blob = malloc(sizeof(*blob));
	blob->map = map;
	blob->size = size;
	blob->offset = 0;

	while (blob->offset < blob->size) {
		uint16_t type = peek_uint16(blob);
		switch (type) {
		case 0x0001:
			parse_string_pool(blob);
			break;
		case 0x0002:
			parse_blob_header(blob);
			break;
		case 0x0200:
			parse_package(blob);
			break;
		case 0x0201:
			parse_type(blob);
			break;
		case 0x0202:
			parse_type_spec(blob);
			break;
		default:
			die("unknown type 0x%04x", type);
		}
	}

	*blob_pp = blob;
}
