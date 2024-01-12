/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#include "dac_string.h"

void dac_strtoupper(register char *str)
{
    while (*str) {
        *str = toupper(*str);
        ++str;
    }
}

char * dac_strchop(const char * str)
{
    char * ret_str = (char*)str;
    char * rstr;
    if(ret_str != NULL){
        while(*ret_str && isspace(*ret_str)){
            ret_str++;
        }
        rstr = (char*)str+strlen(str)-1;
        while(rstr>ret_str && isspace(*rstr)){
            rstr--;
        }
        *(rstr+1) = 0;
    }
    return ret_str;
}


