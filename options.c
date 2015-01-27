#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "options.h"

static void assign_value(const struct option_spec *spec, const char *value)
{
	const char *endp;

	switch (spec->type) {
	case OPT_TYPE_BOOL:
		*((int *)spec->value) = 1;
		break;
	case OPT_TYPE_INTEGER:
		die_if(!value,
		       "option '--%s' requires an argument", spec->long_key);
		*((int *)spec->value) = strtol(value, (char **)&endp, 10);
		die_if(*endp, "unable to convert '%s' to integer", value);
		break;
	case OPT_TYPE_STRING:
		die_if(!value,
		       "option '--%s' requires an argument", spec->long_key);
		*((const char **)spec->value) = value;
		break;
	default:
		die("unexpected spec type %d", spec->type);
	}
}

static void parse_short_option(const struct option_spec *specs, const char *key)
{
	(void)specs;
	(void)key;
	die("not implemented (yet)");
}

static void parse_long_option(const struct option_spec *specs, char *key)
{
	char *value;

	value = strchr(key, '=');
	if (value) {
		*value = '\0';
		value++;
	}

	for (; specs->type != OPT_TYPE_END; specs++) {
		if (!strcmp(specs->long_key, key)) {
			assign_value(specs, value);
			return;
		}
	}
	die("unknown option '--%s'", key);
}

int parse_options(const struct option_spec *specs, int argc, char **argv)
{
	int i;
	const char *arg;

	for (i = 0; i < argc; i++) {
		arg = argv[i];

		if (arg[0] != '-')
			break;

		if (arg[1] != '-')
			parse_short_option(specs, arg);
		else
			parse_long_option(specs, (char *)(arg + 2));
	}

	/* fix up argv, return new number of args */
	memmove(argv, argv + i, (argc - i) * sizeof(char *));
	argv[argc - i + 1] = NULL;
	return argc - i;
}
