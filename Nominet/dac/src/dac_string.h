/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_STRING_FUNCTIONS_H
#define DAC_STRING_FUNCTIONS_H

#include <ctype.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void dac_strtoupper(register char *str);

/* NOTE: return modified pointer! */
char * dac_strchop(const char * str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
