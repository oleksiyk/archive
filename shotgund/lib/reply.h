/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef ASYNC_DNS_REPLY_H
#define ASYNC_DNS_REPLY_H

#include "rfc1035.h"

class AsyncDNSReply
{
private:
    AsyncDNSMemPool * mempool;
    RFC1035_MessageHeader ** headers;
    u_int16_t headerscount;
    u_int8_t * unparsedBuf;
    size_t unparsedBufLen;

    char * tmpBuf;
    RFC1035_MessageQuestion ** questions; //array of questions in response (qdcount size)
    u_int16_t qdcount;
    RFC1035_RR ** answers; //array of answer RR's (ancount size)
    u_int16_t ancount;
    RFC1035_RR ** authorities; //array of authority RR's (nscount size)
    u_int16_t nscount;
    RFC1035_RR ** additionals; //array of additional RR's (arcount size)
    u_int16_t arcount;
    RFC1035_RR ** allrrs; //array of all response RR's (arcount+nscount+ancount)
    u_int16_t allrrsCur;

    typedef RFC1035_RR * (AsyncDNSReply::*parse_RR_func_ptr_t)(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * prevRR[19]; //for linked list
    parse_RR_func_ptr_t parse_RR_funcs[19];

    RFC1035_RR * parse_A_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_NS_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MD_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MF_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_CNAME_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_SOA_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MB_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MG_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MR_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_NULL_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_WKS_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_PTR_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_HINFO_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MINFO_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_MX_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_TXT_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_AAAA_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_NAPTR_RR(u_int16_t rdlength, u_int16_t from);
    RFC1035_RR * parse_SRV_RR(u_int16_t rdlength, u_int16_t from);

    /* uncompress <domain-name> */
    int uncompress(size_t from, size_t tmpBufTo = 0);
    /* how many <character-string>s? */
    int chrstrCount(u_int16_t rdlength, size_t from);
    /* return <character-string>*/
    int chrstrParse(size_t from, size_t tmpBufTo = 0);
    int parseinternal();

public:
    AsyncDNSReply(AsyncDNSMemPool * _mempool);
    virtual ~AsyncDNSReply();

    int addparse(void * _buf, size_t _buflen);
    void cleanup();

    int getAnswerOffset();

    const RFC1035_MessageHeader * getHeader(size_t i = 0) const;
    const RFC1035_MessageQuestion * getQuestion(size_t i = 0) const;
    const RFC1035_RR * getAnswer(size_t i = 0) const;
    const RFC1035_RR * getAuthority(size_t i = 0) const;
    const RFC1035_RR * getAdditional(size_t i = 0) const;
    const RFC1035_RR * getRR(size_t i = 0) const;
    const RFC1035_RR * getLastRRByType(u_int8_t type) const;
};

#endif

