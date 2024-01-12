#ifndef PROBED_STRING_FUNCTIONS_H
#define PROBED_STRING_FUNCTIONS_H

#include <ctype.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void probed_strtoupper(register char *str);

/* NOTE: return modified pointer! */
char * probed_strchop(const char * str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
