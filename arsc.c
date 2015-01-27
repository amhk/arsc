#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmds.h"
#include "common.h"

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
