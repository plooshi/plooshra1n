#ifndef _OPTIONS_H
#define _OPTIONS_H
#include <stdint.h>

extern char xargs_cmd[];
extern char palerain_flags_cmd[];
extern uint64_t palerain_flags;
extern uint64_t plooshrain_flags;

int parse_options(int argc, char* argv[]);

#endif