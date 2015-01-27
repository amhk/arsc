#ifndef ARSC_CMDS_H
#define ARSC_CMDS_H

int cmd_dump(int argc, char **argv);
#ifndef NDEBUG
int cmd_test(int argc, char **argv);
#endif

#endif
