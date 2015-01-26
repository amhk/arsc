#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "filemap.h"

static void map_file0(const char *path, struct mapped_file *map);

#define ZIP_LFH_MAGIC 0x04034b50
#define ZIP_CD_MAGIC 0x02014b50
#define ZIP_EOCD_MAGIC 0x06054b50

/*
 * Limited zip file parser.
 *
 * This code is only intended for retrieval of resources.arsc files in
 * apk files and so makes quite a few assumptions about the zip file
 * structure. For instance, resources.arsc is always stored rather than
 * compressed (by design for performance reasons when loaded by the
 * Android framework) and zip files only consist of a single file.
 *
 * Basic zip format grammar: [LFH file-data]* [CD]* EOCD
 */

/* stripped version of Local File Header */
struct __attribute__ ((__packed__)) zip_lfh {
	uint32_t magic;
	uint8_t padding_1[4];
	uint16_t compression_method;
	uint8_t padding_2[8];
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t filename_length;
	uint16_t extra_length;
	const char filename[0];
};

/* stripped version of Central Directory record */
struct __attribute__ ((__packed__)) zip_cd {
	uint32_t magic;
	uint8_t padding_1[16];
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t filename_length;
	uint16_t extra_length;
	uint16_t comment_length;
	uint8_t padding_2[8];
	uint32_t lfh_offset;
	const char filename[0];
};

/* stripped version of End of Central Directory record */
struct __attribute__ ((__packed__)) zip_eocd {
	uint32_t magic;
	uint8_t padding_1[6];
	uint16_t entry_count; /* assume zip only spans one disk */
	uint32_t cd_size;
	uint32_t cd_offset;
	uint8_t padding_2[2];
};

static const struct zip_eocd *find_eocd(const uint8_t *map, size_t size)
{
	const struct zip_eocd *eocd;

	eocd = (struct zip_eocd *)(map + size - sizeof(*eocd));
	die_if(dtohl(eocd->magic) != ZIP_EOCD_MAGIC,
	       "bad zip eocd magic 0x%08x", dtohl(eocd->magic));
	die_if(dtohs(eocd->entry_count) == 0, "bad entry count 0");

	return eocd;
}

static const struct zip_cd *find_cd_for_entry(const uint8_t *map, size_t size,
					      const struct zip_eocd *eocd,
					      const char *filename)
{
	/*
	 * eocd->cd_offset points to the beginning of eocd->entry_count number
	 * of Central Directories, one per entry. Find the one representing the
	 * requested filename.
	 */
	const struct zip_cd *cd;
	size_t i;

	die_if(eocd->cd_offset > size, "eocd offset too large");
	cd = (struct zip_cd *)(map + dtohl(eocd->cd_offset));
	for (i = 0; i < eocd->entry_count; i++) {
		uint16_t len;

		die_if(dtohl(cd->magic) != ZIP_CD_MAGIC,
		       "bad zip cd magic 0x%08x", dtohl(cd->magic));

		len = dtohs(cd->filename_length);
		if (len == strlen(filename) &&
		    !memcmp(cd->filename, filename, len))
			return cd;
		cd = (const struct zip_cd *)
			((const uint8_t *)cd + sizeof(*cd) + len +
			 dtohs(cd->extra_length) + dtohs(cd->comment_length));
		die_if((const uint8_t *)cd > map + size, "cd outside map");
	}
	die("no entry '%s' found", filename);
	return NULL;
}

static const struct zip_lfh *find_lfh_for_entry(const uint8_t *map, size_t size,
						const struct zip_cd *cd)
{
	const struct zip_lfh *lfh;

	die_if(cd->lfh_offset > size, "lfh offset outside map");
	lfh = (const struct zip_lfh *)(map + cd->lfh_offset);

	die_if(dtohs(lfh->compression_method) != 0,
	       "unhandled compression method %d",
	       dtohs(lfh->compression_method));

	return lfh;
}

static void adjust_map_to_zip_entry(struct mapped_file *map,
				    const char *entry_name)
{
	const struct zip_eocd *eocd;
	const struct zip_cd *cd;
	const struct zip_lfh *lfh;
	uint32_t entry_size;
	uint32_t entry_offset;

	eocd = find_eocd(map->map, map->map_size);
	cd = find_cd_for_entry(map->map, map->map_size, eocd, entry_name);
	lfh = find_lfh_for_entry(map->map, map->map_size, cd);
	entry_size = lfh->compressed_size;
	entry_offset = (const uint8_t *)lfh - (const uint8_t *)map->map +
		sizeof(*lfh) + dtohs(lfh->filename_length) +
		dtohs(lfh->extra_length);

	map->data = (const uint8_t *)map->map + entry_offset;
	map->data_size = entry_size;
}

/*
 * Generic mmap wrappers. Nothing fancy, nothing unexpected.
 */

static void map_file0(const char *path, struct mapped_file *map)
{
	struct stat st;

	if (stat(path, &st) < 0)
		die("stat");
	map->fd = open(path, O_RDONLY);
	if (map->fd < 0)
		die("open");
	map->map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, map->fd, 0);
	if (map->map == MAP_FAILED)
		die("mmap");
	map->map_size = st.st_size;

	map->data = map->map;
	map->data_size = map->map_size;
}

void map_file(const char *path, struct mapped_file *map)
{
	uint32_t magic;

	map_file0(path, map);
	magic = *(const uint32_t *)map->data;
	if (map->data_size > sizeof(uint32_t) &&
	    dtohl(magic) == ZIP_LFH_MAGIC) {
		/* file is likely an apk, modify map->data to point to the
		 * resources.arsc entry withinh the zip */
		adjust_map_to_zip_entry(map, "resources.arsc");
	}
}

void unmap_file(const struct mapped_file *map)
{
	munmap((void *)map->map, map->map_size);
	close(map->fd);
}
