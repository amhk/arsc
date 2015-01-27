#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "common.h"
#include "filemap.h"
#include "options.h"

#ifndef NDEBUG
static struct {
	int b;
	int i;
	const char *s;
} test_opts = { 0, 0, NULL };

static struct option_spec test_option_specs[] = {
	OPT_BOOL('b', "bool", &test_opts.b),
	OPT_INTEGER('i', "integer", &test_opts.i),
	OPT_STRING('s', "string", &test_opts.s),
	OPT_END,
};

static int cmd_test(int argc, char **argv)
{
	argc = parse_options(test_option_specs, argc, argv);
	printf("bool=%d\n", test_opts.b);
	printf("integer=%d\n", test_opts.i);
	printf("string='%s'\n", test_opts.s);
	for (int i = 0; i < argc; i++)
		printf("%d: %s\n", i, argv[i]);
	return 0;
}
#endif

static struct option_spec dump_option_specs[] = {
	OPT_END,
};

static int cmd_dump(int argc, char **argv)
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

int main(int argc, char **argv)
{
	const char *cmd_name;
	int (*cmd_func)(int, char **) = NULL;

	if (argc < 2) {
		fprintf(stderr, "usage: arsc <command> [options]\n");
		exit(1);
	}
	cmd_name = argv[1];

	if (!strcmp(cmd_name, "dump"))
		cmd_func = cmd_dump;
#ifndef NDEBUG
	else if (!strcmp(cmd_name, "test"))
		cmd_func = cmd_test;
#endif

	if (!cmd_func)
		die("no command");

	return cmd_func(argc - 2, argv + 2);
}
