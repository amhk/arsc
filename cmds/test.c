#ifndef NDEBUG
#include <stdio.h>
#include <stdlib.h>

#include "options.h"

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

int cmd_test(int argc, char **argv)
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
