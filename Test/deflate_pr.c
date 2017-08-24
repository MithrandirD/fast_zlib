/*
 * This is a stub file which allows to use custom "C" longest_match function without modification of original Zlib code.
 */

#define ASMV
#include "deflate.c"

/* Prevent error when "longest_match" declared as "extern" but appears "static" */
#undef local
#define local

/* Include our match algorithm */
#include "../Sources/match_pr.h"

void match_init()
{
}
