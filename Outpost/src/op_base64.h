/*  Copyright (C) 2003 FOSS-On-Line <http://www.foss.kharkov.ua>,
*   Aleksey Krivoshey <krivoshey@users.sourceforge.net>
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

#ifndef OUTPOST_BASE64_H
#define OUTPOST_BASE64_H

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int op_base64decode_len(const char *bufcoded);
int op_base64decode(char *bufplain, const char *bufcoded);

int op_base64encode_len(int plainlen);
int op_base64encode(char *encoded, const char *plain, int plainlen);

// out must be 2*inlen+1 bytes length
unsigned char *op_base16(const unsigned char *in, size_t inlen, unsigned char * out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
