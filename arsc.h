#ifndef ARSC_ARSC_H
#define ARSC_ARSC_H

#include <stddef.h>
#include <stdint.h>

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
 * Wrapper structs. These are writeable during parsing, but should be
 * considered read-only afterwards.
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


#endif
