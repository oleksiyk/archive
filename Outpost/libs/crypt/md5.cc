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

#include "md5.h"

#define MD5_ROL(w,n) ( (w) << (n) | ( (w) >> (32-(n)) ) )

static u_int32_t T[64]={
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
0xd62f105d, 0x2441453, 0xd8a1e681, 0xe7d3fbc8,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

void MD5::context_init()
{
    if (sizeof(u_int32_t) != 4){
        fprintf(stderr, "MD5 calculation not possible\n");
        abort();
    }

    c.A=0x67452301;
    c.B=0xefcdab89;
    c.C=0x98badcfe;
    c.D=0x10325476;

    c.blk_ptr=0;
}

void MD5::context_hash(const unsigned char blk[MD5_BLOCK_SIZE])
{
    u_int32_t       x[16];
    unsigned        i, j;
    u_int32_t       A, B, C, D;
    u_int32_t       zz;

    for (i=j=0; i<16; i++){
        u_int32_t w = (u_int32_t)blk[j++];

        w |= (u_int32_t)blk[j++] << 8;
        w |= (u_int32_t)blk[j++] << 16;
        w |= (u_int32_t)blk[j++] << 24;
        x[i]= w;
    }

#define F(X,Y,Z) ( ((X) & (Y)) | ( (~(X)) & (Z)))
#define G(X,Y,Z) ( ((X) & (Z)) | ( (Y) & (~(Z))))
#define H(X,Y,Z) ( (X) ^ (Y) ^ (Z) )
#define I(X,Y,Z) ( (Y) ^ ( (X) | (~(Z))))

    A=c.A;
    B=c.B;
    C=c.C;
    D=c.D;

#define ROUND1(a,b,c,d,k,s,i) { zz=(a + F(b,c,d) + x[k] + T[i]); a=b+MD5_ROL(zz,s); }

    ROUND1(A,B,C,D,0,7,0);
    ROUND1(D,A,B,C,1,12,1);
    ROUND1(C,D,A,B,2,17,2);
    ROUND1(B,C,D,A,3,22,3);
    ROUND1(A,B,C,D,4,7,4);
    ROUND1(D,A,B,C,5,12,5);
    ROUND1(C,D,A,B,6,17,6);
    ROUND1(B,C,D,A,7,22,7);
    ROUND1(A,B,C,D,8,7,8);
    ROUND1(D,A,B,C,9,12,9);
    ROUND1(C,D,A,B,10,17,10);
    ROUND1(B,C,D,A,11,22,11);
    ROUND1(A,B,C,D,12,7,12);
    ROUND1(D,A,B,C,13,12,13);
    ROUND1(C,D,A,B,14,17,14);
    ROUND1(B,C,D,A,15,22,15);

#define ROUND2(a,b,c,d,k,s,i) { zz=(a + G(b,c,d) + x[k] + T[i]); a = b + MD5_ROL(zz,s); }

    ROUND2(A,B,C,D,1,5,16);
    ROUND2(D,A,B,C,6,9,17);
    ROUND2(C,D,A,B,11,14,18);
    ROUND2(B,C,D,A,0,20,19);
    ROUND2(A,B,C,D,5,5,20);
    ROUND2(D,A,B,C,10,9,21);
    ROUND2(C,D,A,B,15,14,22);
    ROUND2(B,C,D,A,4,20,23);
    ROUND2(A,B,C,D,9,5,24);
    ROUND2(D,A,B,C,14,9,25);
    ROUND2(C,D,A,B,3,14,26);
    ROUND2(B,C,D,A,8,20,27);
    ROUND2(A,B,C,D,13,5,28);
    ROUND2(D,A,B,C,2,9,29);
    ROUND2(C,D,A,B,7,14,30);
    ROUND2(B,C,D,A,12,20,31);

#define ROUND3(a,b,c,d,k,s,i) { zz=(a + H(b,c,d) + x[k] + T[i]); a = b + MD5_ROL(zz,s); }

    ROUND3(A,B,C,D,5,4,32);
    ROUND3(D,A,B,C,8,11,33);
    ROUND3(C,D,A,B,11,16,34);
    ROUND3(B,C,D,A,14,23,35);
    ROUND3(A,B,C,D,1,4,36);
    ROUND3(D,A,B,C,4,11,37);
    ROUND3(C,D,A,B,7,16,38);
    ROUND3(B,C,D,A,10,23,39);
    ROUND3(A,B,C,D,13,4,40);
    ROUND3(D,A,B,C,0,11,41);
    ROUND3(C,D,A,B,3,16,42);
    ROUND3(B,C,D,A,6,23,43);
    ROUND3(A,B,C,D,9,4,44);
    ROUND3(D,A,B,C,12,11,45);
    ROUND3(C,D,A,B,15,16,46);
    ROUND3(B,C,D,A,2,23,47);

#define ROUND4(a,b,c,d,k,s,i) { zz=(a + I(b,c,d) + x[k] + T[i]); a = b + MD5_ROL(zz,s); }

    ROUND4(A,B,C,D,0,6,48);
    ROUND4(D,A,B,C,7,10,49);
    ROUND4(C,D,A,B,14,15,50);
    ROUND4(B,C,D,A,5,21,51);
    ROUND4(A,B,C,D,12,6,52);
    ROUND4(D,A,B,C,3,10,53);
    ROUND4(C,D,A,B,10,15,54);
    ROUND4(B,C,D,A,1,21,55);
    ROUND4(A,B,C,D,8,6,56);
    ROUND4(D,A,B,C,15,10,57);
    ROUND4(C,D,A,B,6,15,58);
    ROUND4(B,C,D,A,13,21,59);
    ROUND4(A,B,C,D,4,6,60);
    ROUND4(D,A,B,C,11,10,61);
    ROUND4(C,D,A,B,2,15,62);
    ROUND4(B,C,D,A,9,21,63);

    c.A += A;
    c.B += B;
    c.C += C;
    c.D += D;
}

void MD5::context_hashstream(const void *p, unsigned l)
{
    const unsigned char *cp=(const unsigned char *)p;
    unsigned ll;

    while (l){
        if (c.blk_ptr == 0 && l >= MD5_BLOCK_SIZE){
            context_hash(cp);
            cp += MD5_BLOCK_SIZE;
            l -= MD5_BLOCK_SIZE;
            continue;
        }

        ll=l;
        if (ll > MD5_BLOCK_SIZE - c.blk_ptr){
            ll=MD5_BLOCK_SIZE - c.blk_ptr;
        }
        memcpy(c.blk + c.blk_ptr, cp, ll);
        c.blk_ptr += ll;
        cp += ll;
        l -= ll;
        if (c.blk_ptr >= MD5_BLOCK_SIZE) {
            context_hash(c.blk);
            c.blk_ptr=0;
        }
    }
}

void MD5::context_endstream(unsigned long ll)
{
    unsigned char buf[8];
    static unsigned char zero[MD5_BLOCK_SIZE-8];
    u_int32_t l;

    buf[0]=0x80;
    context_hashstream(buf, 1);

    while (c.blk_ptr != MD5_BLOCK_SIZE - 8){
        if (c.blk_ptr > MD5_BLOCK_SIZE - 8){
            context_hashstream(zero, MD5_BLOCK_SIZE - c.blk_ptr);
            continue;
        }
        context_hashstream(zero, MD5_BLOCK_SIZE - 8 - c.blk_ptr);
    }

    l= ll;

    l <<= 3;

    buf[0]=l;
    l >>= 8;
    buf[1]=l;
    l >>= 8;
    buf[2]=l;
    l >>= 8;
    buf[3]=l;

    l= ll;
    l >>= 29;
    buf[4]=l;
    l >>= 8;
    buf[5]=l;
    l >>= 8;
    buf[6]=l;
    l >>= 8;
    buf[7]=l;

    context_hashstream(buf, 8);
}

void MD5::context_digest(Digest d)
{
    unsigned char *dp=d;
    u_int32_t w;

#define PUT(c) (w=(c), *dp++ = w, w >>= 8, *dp++ = w, w >>= 8, *dp++ = w, w >>= 8, *dp++ = w)
    PUT(c.A);
    PUT(c.B);
    PUT(c.C);
    PUT(c.D);
#undef  PUT
}

void MD5::context_restore(const Digest d)
{
    const unsigned char *dp = (unsigned char *)d+MD5_DIGEST_SIZE;
    u_int32_t w;

#define GET w=*--dp; w=(w << 8) | *--dp; w=(w << 8) | *--dp; w=(w << 8) | *--dp;

    GET
    c.D=w;
    GET
    c.C=w;
    GET
    c.B=w;
    GET
    c.A=w;
    c.blk_ptr=0;
}

void MD5::digest(const void *msg, unsigned int len, Digest d)
{
    context_init();
    context_hashstream(msg, len);
    context_endstream(len);
    context_digest(d);
}

static char base64[]= "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

char * MD5::crypt(const char *pw, const char *salt)
{
    Digest digest;
    unsigned pwl=strlen(pw);
    unsigned l;
    unsigned i, j;
    char *p;
    char * buffer = (char*)malloc(100);
    if(buffer == NULL)
	return NULL;

    if (*salt == '$' && salt[1] == '1' && salt[2] == '$')
        salt += 3;

    for (l=0; l<8 && salt[l] && salt[l] != '$'; l++);

    context_init();
    context_hashstream(pw, pwl);
    context_hashstream(salt, l);
    context_hashstream(pw, pwl);
    context_endstream(pwl*2+l);
    context_digest(digest);

    context_init();
    context_hashstream(pw, pwl);
    context_hashstream("$1$", 3);
    context_hashstream(salt, l);

    for (i=pwl; i; ) {
        j=i;
        if (j > 16) j=16;
        context_hashstream(digest, j);
        i -= j;
    }

    j=pwl*2+l+3;

    for (i=pwl; i; i >>= 1){
        context_hashstream((i & 1) ? "": pw, 1);
        ++j;
    }

    context_endstream(j);
    context_digest(digest);

    for (i=0; i<1000; i++){
        j=0;

        context_init();
        if (i & 1){
            context_hashstream(pw, pwl);
            j += pwl;
        } else {
            context_hashstream(digest, 16);
            j += 16;
        }
        if (i % 3) {
            context_hashstream(salt, l);
            j += l;
        }
        if (i % 7){
            context_hashstream(pw, pwl);
            j += pwl;
        }
        if (i & 1){
            context_hashstream(digest, 16);
            j += 16;
        } else {
            context_hashstream(pw, pwl);
            j += pwl;
        }

        context_endstream(j);
        context_digest(digest);
    }

    strcpy(buffer, "$1$");
    strncat(buffer, salt, l);
    strcat(buffer, "$");

    p=buffer+strlen(buffer);
    unsigned char *d = digest;
    for (i=0; i<5; i++){
        j= (d[i] << 16) | (d[i+6] << 8) | d[i == 4 ? 5:12+i];
        *p++= base64[j & 63] ; j=j >> 6;
        *p++= base64[j & 63] ; j=j >> 6;
        *p++= base64[j & 63] ; j=j >> 6;
        *p++= base64[j & 63];
    }
    j=digest[11];
    *p++ = base64[j & 63]; j=j >> 6;
    *p++ = base64[j & 63];
    *p=0;

    return buffer;
}

void MD5::hmac(const unsigned char *key, size_t keylen, const unsigned char *data, size_t datalen, Digest d)
{
    unsigned char k_ipad[65];
    unsigned char k_opad[65];

    Digest tk;
    // if key is longer than 64 bytes reset it to key=MD5(key)
    if(keylen>64){
        digest(key, keylen, tk);
        key = tk;
        keylen = MD5_DIGEST_SIZE;
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
    context_hashstream(d, MD5_DIGEST_SIZE);
    context_endstream(64+MD5_DIGEST_SIZE);
    context_digest(d);
}
