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



#include        "sha1.h"

#define K0 0x5A827999
#define K1 0x6ED9EBA1
#define K2 0x8F1BBCDC
#define K3 0XCA62C1D6

#define K20(x)  x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x

static u_int32_t K[80] = { K20(K0), K20(K1), K20(K2), K20(K3) };

void SHA1::context_init()
{
    if (sizeof(u_int32_t) != 4)
        abort();

    c.H[0] = 0x67452301;
    c.H[1] = 0xEFCDAB89;
    c.H[2] = 0x98BADCFE;
    c.H[3] = 0x10325476;
    c.H[4] = 0xC3D2E1F0;
    c.blk_ptr=0;
}

void SHA1::context_hash(const unsigned char blk[SHA1_BLOCK_SIZE])
{
    u_int32_t       A,B,C,D,E;
    u_int32_t       TEMP;
    u_int32_t       W[80];
    unsigned        i, t;

#define f(t,B,C,D)      ( \
        (t) < 20 ? ( (B) & (C) ) | ( (~(B)) & (D) ) : \
        (t) >= 40 && (t) < 60 ? ( (B) & (C) ) | ( (B) & (D) ) | ( (C) & (D) ):\
                (B) ^ (C) ^ (D) )

#define S(a,b) ( ((u_int32_t)(a) << (b)) | ((u_int32_t)(a) >> (32 - (b))))

    for (i=t=0; t<16; t++) {
        W[t]= blk[i]; i++;
        W[t] = (W[t] << 8) | blk[i]; i++;
        W[t] = (W[t] << 8) | blk[i]; i++;
        W[t] = (W[t] << 8) | blk[i]; i++;
    }

    for (t=16; t<80; t++){
        TEMP= W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16];
        W[t]= S(TEMP, 1);
    }

    A=c.H[0];
    B=c.H[1];
    C=c.H[2];
    D=c.H[3];
    E=c.H[4];

    for (t=0; t<80; t++){
        TEMP = S(A,5);
        TEMP += f(t, B, C, D);
        TEMP += E;
        TEMP += W[t];
        TEMP += K[t];

        E=D;
        D=C;
        C= S(B, 30);
        B=A;
        A=TEMP;
    }

    c.H[0] += A;
    c.H[1] += B;
    c.H[2] += C;
    c.H[3] += D;
    c.H[4] += E;
}

void SHA1::context_hashstream(const void *p, unsigned l)
{
    const unsigned char *cp=(const unsigned char *)p;
    unsigned ll;

    while (l){
        if (c.blk_ptr == 0 && l >= SHA1_BLOCK_SIZE){
            context_hash(cp);
            cp += SHA1_BLOCK_SIZE;
            l -= SHA1_BLOCK_SIZE;
            continue;
        }

        ll=l;
        if (ll > SHA1_BLOCK_SIZE - c.blk_ptr)
            ll=SHA1_BLOCK_SIZE - c.blk_ptr;
        memcpy(c.blk + c.blk_ptr, cp, ll);
        c.blk_ptr += ll;
        cp += ll;
        l -= ll;
        if (c.blk_ptr >= SHA1_BLOCK_SIZE){
            context_hash(c.blk);
            c.blk_ptr=0;
        }
    }
}

void SHA1::context_endstream(unsigned long l)
{
    unsigned char buf[8];
    static unsigned char zero[SHA1_BLOCK_SIZE-8];

    buf[0]=0x80;
    context_hashstream(&buf, 1);
    while (c.blk_ptr != SHA1_BLOCK_SIZE-8){
        if (c.blk_ptr > SHA1_BLOCK_SIZE-8){
            context_hashstream(zero, SHA1_BLOCK_SIZE - c.blk_ptr);
            continue;
        }
        context_hashstream(zero, SHA1_BLOCK_SIZE-8-c.blk_ptr);
    }

    l *= 8;
    buf[7] = l;
    buf[6] = (l >>= 8);
    buf[5] = (l >>= 8);
    buf[4] = (l >> 8);
    buf[3]=buf[2]=buf[1]=buf[0]=0;

    context_hashstream(buf, 8);
}

void SHA1::context_digest(SHA1::Digest d)
{
    unsigned char *dp=d + SHA1_DIGEST_SIZE;
    unsigned i;

    u_int32_t w = 0;
    for ( i=5; i; ) {
        w = c.H[--i];
        *--dp=w; w >>= 8;
        *--dp=w; w >>= 8;
        *--dp=w; w >>= 8;
        *--dp=w;
    }
}

void SHA1::context_restore(const SHA1::Digest d)
{
    const unsigned char *dp=d;
    unsigned i;

    u_int32_t w = 0;
    for (i=0; i<5; i++) {
        w= *dp++;

        w=(w << 8) | *dp++;
        w=(w << 8) | *dp++;
        w=(w << 8) | *dp++;
        c.H[i]=w;
    }
    c.blk_ptr=0;
}

void SHA1::digest(const void *msg, unsigned len, SHA1::Digest d)
{
    context_init();
    context_hashstream(msg, len);
    context_endstream(len);
    context_digest(d);
}

void SHA1::hmac(const unsigned char *key, size_t keylen, const unsigned char *data, size_t datalen, Digest d)
{
    unsigned char k_ipad[65];
    unsigned char k_opad[65];

    Digest tk;
    // if key is longer than 64 bytes reset it to key=SHA1(key)
    if(keylen>64){
        digest(key, keylen, tk);
        key = tk;
        keylen = SHA1_DIGEST_SIZE;
    }

    memset(k_ipad, 0, sizeof k_ipad);
    memset(k_opad, 0, sizeof k_opad);
    memcpy(k_ipad, key, keylen);
    memcpy(k_opad, key, keylen);

    for(int i = 0; i<64; i++){
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    context_init();
    context_hashstream(k_ipad, 64);
    context_hashstream(data, datalen);
    context_endstream(64+datalen);
    context_digest(d);

    context_init();
    context_hashstream(k_opad, 64);
    context_hashstream(d, SHA1_DIGEST_SIZE);
    context_endstream(64+SHA1_DIGEST_SIZE);
    context_digest(d);
}

