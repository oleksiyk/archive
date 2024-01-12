/*  Copyright (C) 2003 Aleksey Krivoshey <krivoshey@users.sourceforge.net>
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


/* Based on code from courier-imap server */
/*
** Copyright 1998 - 1999 Double Precision, Inc.
** See COPYING for distribution information.
*/



#ifndef OUTPOST_SHA1_H
#define OUTPOST_SHA1_H

#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define	SHA1_DIGEST_SIZE	20
#define	SHA1_BLOCK_SIZE		64

class SHA1
{
public:
    typedef unsigned char Digest[SHA1_DIGEST_SIZE];

private:
    struct Context
    {
	u_int32_t	H[5];
	unsigned char blk[SHA1_BLOCK_SIZE];
	unsigned blk_ptr;
    };
    struct Context c;

    void context_init();
    void context_hash(const unsigned char[SHA1_BLOCK_SIZE]);
    void context_hashstream(const void *, unsigned);
    void context_endstream(unsigned long);
    void context_digest(Digest);
    void context_restore(const Digest);

public:
    /* get SHA1 hash, result is in provided Digest */
    void digest(const void *msg, unsigned int len, Digest d);

    /* HMAC_SHA-1 */
    void hmac(const unsigned char *key, size_t keylen, const unsigned char *data, size_t datalen, Digest d);
};

#endif
