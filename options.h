#ifndef ARSC_OPTIONS_H
#define ARSC_OPTIONS_H

struct option_spec {
	enum {
		OPT_TYPE_END,
		OPT_TYPE_BOOL,
		OPT_TYPE_INTEGER,
		OPT_TYPE_STRING,
	} type;
	char short_key;
	const char *long_key;
	void *value;
};

#define OPT_END { OPT_TYPE_END, 0, NULL, NULL }
#define OPT_BOOL(s, l, v) { OPT_TYPE_BOOL, (s), (l), (v) }
#define OPT_INTEGER(s, l, v) { OPT_TYPE_INTEGER, (s), (l), (v) }
#define OPT_STRING(s, l, v) { OPT_TYPE_STRING, (s), (l), (v) }

/*
 * Scan argv for options matching the given specification. Stop at the
 * first option with no leading dash, and update argv to only include
 * that option and any following it. Return the new size of argv.
 */
int parse_options(const struct option_spec *spec, int argc, char **argv);

#endif
