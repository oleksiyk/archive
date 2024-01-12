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


#ifndef ASYNC_DNS_RFC1035_H
#define ASYNC_DNS_RFC1035_H

#include "config.h"

/*
#define DEBUG
#ifdef DEBUG
    #define return fprintf(stderr, __FILE__": %d\n", __LINE__); return
#endif
*/

#ifdef WIN32
#include <winsock.h>
#include <time.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <pthread.h>

#ifdef WIN32
#define INET_ADDRSTRLEN 16
#define socklen_t int
#define close closesocket
#define strcasecmp stricmp
#define snprintf _snprintf
#define recvfrom(fd, p, lp, fl, fr, frl) recvfrom(fd, (char*)p, lp, fl, fr, frl)
#define sendto(fd, p, lp, fl, ft, ftl) sendto(fd, (char*)p, lp, fl, ft, ftl)
typedef unsigned char  u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int   u_int32_t;
#endif


#ifdef HAVE_IPV6
    #define RFC1035_sockaddr_in sockaddr_in6
    #define RFC1035_AF_INET AF_INET6
    #define RFC1035_PF_INET PF_INET6
    #define RFC1035_sin_family sin6_family
    #define RFC1035_sin_port sin6_port
    #define RFC1035_sin_addr sin6_addr 
    #define RFC1035_in_addr in6_addr
    #define RFC1035_INET_ADDRSTRLEN INET6_ADDRSTRLEN
#else
    #define RFC1035_sockaddr_in sockaddr_in
    #define RFC1035_AF_INET AF_INET
    #define RFC1035_PF_INET PF_INET
    #define RFC1035_sin_family sin_family
    #define RFC1035_sin_port sin_port
    #define RFC1035_sin_addr sin_addr
    #define RFC1035_in_addr in_addr
    #define RFC1035_INET_ADDRSTRLEN INET_ADDRSTRLEN
#endif


#pragma pack(1)

struct RFC1035_MessageHeader
{
    u_int16_t   id;
    u_int8_t    flags_hi;
    u_int8_t    flags_lo;
    u_int8_t    qdcount_hi;
    u_int8_t    qdcount_lo;
    u_int8_t    ancount_hi;
    u_int8_t    ancount_lo;
    u_int8_t    nscount_hi;
    u_int8_t    nscount_lo;
    u_int8_t    arcount_hi;
    u_int8_t    arcount_lo;

    inline u_int8_t get_qr()const{ return ((flags_hi & 128) >> 7); }
    inline u_int8_t get_opcode()const{ return ((flags_hi & 120) >> 3);}
    inline void set_opcode(u_int8_t opcode){ flags_hi |= ((opcode << 3) & 120); }
    inline u_int8_t get_aa()const{ return ((flags_hi & 4 ) >> 2); }
    inline u_int8_t get_tc()const{ return ((flags_hi & 2 ) >> 1); }
    inline void set_rd(u_int8_t rd){ flags_hi |= (rd & 1); }
    inline u_int8_t get_ra()const{ return (flags_lo & 128); }
    inline u_int8_t get_rcode()const{ return (flags_lo & 15); }
    inline void set_rcode(u_int8_t rcode){ flags_lo = (flags_lo & ~15)|(rcode & 15); }
    inline u_int16_t get_qdcount()const{ return qdcount_lo + (((u_int16_t)qdcount_hi) << 8); }
    inline void set_qdcount(u_int16_t qdcount){ qdcount_hi = qdcount >> 8; qdcount_lo = (u_int8_t)qdcount; }
    inline u_int16_t get_ancount()const{ return ancount_lo + (((u_int16_t)ancount_hi) << 8); }
    inline u_int16_t get_nscount()const{ return nscount_lo + (((u_int16_t)nscount_hi) << 8); }
    inline u_int16_t get_arcount()const{ return arcount_lo + (((u_int16_t)arcount_hi) << 8); }
};

struct AsyncDNSMessage
{
    RFC1035_MessageHeader header;
    unsigned char buf[500];
};

#pragma pack()

struct RFC1035_MessageQuestion
{
    char *      qname; //parsed domain name value, not the sequence of labels
    u_int16_t   qtype;
    u_int16_t   qclass;
    u_int16_t   header_id; //NOT RFC1035 field! associates _question_ with corresponding response header in reply 
                           //( since reply (AsyncDNSReply) can contain more then 1 real reponses )
};

/* 
*    RRs of the same type are linked in unidirectional list e.g:
*        NULL <- MX_RR_1  <- MX_RR_2 <- MX_RR_last
*/

struct RFC1035_RR
{
    char *      name; //parsed domain name value, not the sequence of labels
    u_int16_t   type;
    u_int16_t   rr_class;
    u_int16_t   ttl;
    const RFC1035_RR * prevRR; //NOT RFC1035 field! points to previous RR of the same type
    u_int16_t   header_id; //NOT RFC1035 field! associates RR with corresponding response header in reply 
                           //( since reply (AsyncDNSReply) can contain more then 1 real reponses )
};

struct RFC1035_A_RR : public RFC1035_RR
{
    in_addr address;
};

struct RFC1035_AAAA_RR : public RFC1035_RR
{
    in6_addr address;
};

struct RFC1035_CNAME_RR : public RFC1035_RR
{
    u_int8_t    resolver_checked; //NOT RFC1035 field!, used by AsyncDNSResolver when resolving CNAME's
    char *      cname;    
};

struct RFC1035_HINFO_RR : public RFC1035_RR
{
    char * cpu;
    char * os;
};

struct RFC1035_MB_RR : public RFC1035_RR
{
    char * madname;
};

struct RFC1035_MD_RR : public RFC1035_RR
{
    char * madname;
};

struct RFC1035_MF_RR : public RFC1035_RR
{
    char * madname;
};

struct RFC1035_MG_RR : public RFC1035_RR
{
    char * mgmname;
};

struct RFC1035_MINFO_RR : public RFC1035_RR
{
    char * rmailbx;
    char * emailbx;
};

struct RFC1035_MR_RR : public RFC1035_RR
{
    char * newname;
};

struct RFC1035_MX_RR : public RFC1035_RR
{
    u_int8_t    resolver_checked; //NOT RFC1035 field!, used by AsyncDNSResolver when resolving CNAME's
    u_int16_t   preference;
    char *      exchange;    
};

struct RFC1035_NULL_RR : public RFC1035_RR
{
    unsigned char * anything;
};

struct RFC1035_NS_RR : public RFC1035_RR
{
    char * nsdname;
};

struct RFC1035_PTR_RR : public RFC1035_RR
{
    char * ptrdname;
};

struct RFC1035_SOA_RR : public RFC1035_RR
{
    char *      mname;
    char *      rname;
    u_int32_t   serial;
    u_int32_t   refresh;
    u_int32_t   retry;
    u_int32_t   expire;
    u_int32_t   minimum;
};

struct RFC1035_TXT_RR : public RFC1035_RR
{
    char ** txt_data; //array of c-strings, array is terminated with NULL
};

struct RFC1035_WKS_RR : public RFC1035_RR
{
    in_addr     address;
    u_int8_t    protocol;
    u_int8_t    *bitmap;
};


#define RFC1035_OPCODE_QUERY        0
#define RFC1035_OPCODE_IQUERY       1
#define RFC1035_OPCODE_STATUS       2

#define RFC1035_RCODE_NOERROR       0
#define RFC1035_RCODE_FORMAT        1
#define RFC1035_RCODE_SERVFAIL      2
#define RFC1035_RCODE_NXDOMAIN      3
#define RFC1035_RCODE_UNIMPLEMENTED 4
#define RFC1035_RCODE_REFUSED       5

#define RFC1035_TYPE_A              1
#define RFC1035_TYPE_NS             2
#define RFC1035_TYPE_MD             3
#define RFC1035_TYPE_MF             4
#define RFC1035_TYPE_CNAME          5
#define RFC1035_TYPE_SOA            6
#define RFC1035_TYPE_MB             7
#define RFC1035_TYPE_MG             8
#define RFC1035_TYPE_MR             9
#define RFC1035_TYPE_NULL           10
#define RFC1035_TYPE_WKS            11
#define RFC1035_TYPE_PTR            12
#define RFC1035_TYPE_HINFO          13
#define RFC1035_TYPE_MINFO          14
#define RFC1035_TYPE_MX             15
#define RFC1035_TYPE_TXT            16
#define RFC1035_TYPE_AAAA           28
#define RFC1035_TYPE_AXFR           252
#define RFC1035_TYPE_MAILB          253
#define RFC1035_TYPE_MAILA          254
#define RFC1035_TYPE_ANY            255

#define RFC1035_CLASS_IN            1
#define RFC1035_CLASS_CSNET         2
#define RFC1035_CLASS_CHAOS         3
#define RFC1035_CLASS_HESIOD        4
#define RFC1035_CLASS_ANY           255

#define RFC1035_RESOLVE_RECURSIVE   1

#define RFC1035_MAX_LABELSIZE       63
#define RFC1035_MAX_NAMESIZE        255
#define RFC1035_MAX_UDPSIZE         512

#define RFC1035_MAX_REFERRAL_LINKS      20
#define RFC1035_MAX_CNAME_TRANSLATIONS  8

class AsyncDNSResolver;
class AsyncDNSReply;
class AsyncDNSMemPool;

#include "reply.h"
#include "resolver.h"
#include "mempool.h"

#endif

