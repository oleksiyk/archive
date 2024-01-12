/*  Copyright (C) 2004 Aleksey Krivoshey
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef HTTP_FUNCTIONS_H
#define HTTP_FUNCTIONS_H

#include <sys/time.h>
#include <unistd.h>
//#include <linux/types.h>
#include <fnmatch.h>

struct HTTPContentType {
    const char *name;               /* postfix of file */
    const char *type;               /* matching content-type */
    const char *encoding;   /* matching content-encoding */
    const char *encoding11; /* matching content-encoding (HTTP/1.1) */
};


const struct HTTPContentType * get_content_type(const char * filename);

/*
struct ltstr
{
    bool operator()(const char* s1, const char* s2) const
    {
        //return strcmp(s1, s2) < 0; //for map
        return strcmp(s1, s2) == 0; //for hash_map
    }
};
*/

void embedhttp_makeTimeString(char * buf, time_t clock = 0);

int embedhttp_base64decode_len(const char *bufcoded);
int embedhttp_base64decode(char *bufplain, const char *bufcoded);
int embedhttp_base64decode_binary(unsigned char *bufplain, const char *bufcoded);
int embedhttp_base64encode_len(int len);
int embedhttp_base64encode(char *encoded, const char *string, int len);
int embedhttp_base64encode_binary(char *encoded, const unsigned char *string, int len);

unsigned char embedhttp_hex2char(unsigned char c);

char * embedhttp_chop_string(const char * str);

#endif
