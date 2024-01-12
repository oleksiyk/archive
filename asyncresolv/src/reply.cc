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


#include "rfc1035.h"

AsyncDNSReply::AsyncDNSReply(AsyncDNSMemPool * _mempool):
    mempool(_mempool), headers(NULL), headerscount(0),
    tmpBuf(NULL),
    questions(NULL), qdcount(0),
    answers(NULL), ancount(0),
    authorities(NULL), nscount(0),
    additionals(NULL), arcount(0),
    allrrs(NULL), allrrsCur(0)
{
    parse_RR_funcs[0] = &AsyncDNSReply::parse_A_RR;
    parse_RR_funcs[1] = &AsyncDNSReply::parse_NS_RR;
    parse_RR_funcs[2] = &AsyncDNSReply::parse_MD_RR;
    parse_RR_funcs[3] = &AsyncDNSReply::parse_MF_RR;
    parse_RR_funcs[4] = &AsyncDNSReply::parse_CNAME_RR;
    parse_RR_funcs[5] = &AsyncDNSReply::parse_SOA_RR;
    parse_RR_funcs[6] = &AsyncDNSReply::parse_MB_RR;
    parse_RR_funcs[7] = &AsyncDNSReply::parse_MG_RR;
    parse_RR_funcs[8] = &AsyncDNSReply::parse_MR_RR;
    parse_RR_funcs[9] = &AsyncDNSReply::parse_NULL_RR;
    parse_RR_funcs[10] = &AsyncDNSReply::parse_WKS_RR;
    parse_RR_funcs[11] = &AsyncDNSReply::parse_PTR_RR;
    parse_RR_funcs[12] = &AsyncDNSReply::parse_HINFO_RR;
    parse_RR_funcs[13] = &AsyncDNSReply::parse_MINFO_RR;
    parse_RR_funcs[14] = &AsyncDNSReply::parse_MX_RR;
    parse_RR_funcs[15] = &AsyncDNSReply::parse_TXT_RR;
    parse_RR_funcs[16] = &AsyncDNSReply::parse_AAAA_RR;

    for(size_t i = 0; i < 16; i++){
        prevRR[i] = NULL;
    }
}

AsyncDNSReply::~AsyncDNSReply()
{
}

int AsyncDNSReply::uncompress(size_t from, size_t tmpBufTo)
{
    if( from >= (unparsedBufLen-1)){
        return -1;
    }

    u_int8_t * p = &unparsedBuf[from];
    size_t k = tmpBufTo;
    u_int8_t c = *p++;

    if(c == 0){
        tmpBuf[k] = '.';
        return from+k+1;
    }


    while(*p){
        if((from+k) >= unparsedBufLen){
            return -1;
        }
        if(k >= RFC1035_MAX_NAMESIZE){
            return -1;
        }
        if(c & 192){ //compression pointer
            u_int16_t frp = (((u_int16_t)(c&63))<<8) + *p;
            if(uncompress(frp, k) == -1){
                return -1;
            }
            return from+k+2;
        }
        if(c > RFC1035_MAX_LABELSIZE){
            return -1;
        }
        if(!c){
            tmpBuf[k++] = '.';
            c = *p++;
        } else {
            tmpBuf[k++] = *p++;
            c--;
        }
    }
    tmpBuf[k++] = 0;

    return from+k+1;
}

int AsyncDNSReply::chrstrCount(u_int16_t rdlength, size_t from)
{
    if( from >= (unparsedBufLen-1)){
        return -1;
    }
    
    int count = 0;
    u_int8_t * p = &unparsedBuf[from];
    size_t k = 1;
    u_int8_t c = *p++;

    while(k < rdlength){
        if((from+k) >= unparsedBufLen){
            return -1;
        }
        if(k >= RFC1035_MAX_NAMESIZE){
            return -1;
        }
        k++; p++; c--;
        if(!c){
            count++;
            c = *p++;
            k++;
        }
    }

    return count;
}

int AsyncDNSReply::chrstrParse(size_t from, size_t tmpBufTo)
{
    if( from >= (unparsedBufLen-1)){
        return -1;
    }

    u_int8_t * p = &unparsedBuf[from];
    size_t k = tmpBufTo;
    u_int8_t c = *p++;

    while(c){
        if((from+k) >= unparsedBufLen){
            return -1;
        }
        if(k >= RFC1035_MAX_NAMESIZE){
            return -1;
        }
        tmpBuf[k++] = *p++;
        c--;
    }
    tmpBuf[k++] = 0;

    return from+k;
}

int AsyncDNSReply::parseinternal()
{
    int k = sizeof(RFC1035_MessageHeader);

    u_int16_t qdc = headers[headerscount-1]->get_qdcount();
    u_int16_t anc = headers[headerscount-1]->get_ancount();
    u_int16_t nsc = headers[headerscount-1]->get_nscount();
    u_int16_t arc = headers[headerscount-1]->get_arcount();
    size_t i = 0;

    if(qdc){

        for(i = 0; i<qdc; i++){
            k = uncompress(k);
            if(k == -1 || (unsigned)(k+3) >= unparsedBufLen){
                return -1;
            }
            questions[qdcount+i] = (RFC1035_MessageQuestion*)mempool->alloc(sizeof(RFC1035_MessageQuestion));
            questions[qdcount+i]->qname = mempool->strdup(tmpBuf);
            questions[qdcount+i]->qtype = (((u_int16_t)unparsedBuf[k])<<8) + unparsedBuf[k+1];
            questions[qdcount+i]->qclass = (((u_int16_t)unparsedBuf[k+2])<<8) + unparsedBuf[k+3];
            questions[qdcount+i]->header_id = headers[headerscount-1]->id;
            k+=4;
        }
        qdcount += i;
    }

    if(headers[headerscount-1]->get_rcode() != RFC1035_RCODE_NOERROR){
        return 0;
    }

    if(!qdc) {
        return -1; //no questions in response?
    }

    if(!(anc+nsc+arc) && headers[headerscount-1]->get_aa()){ //authoritaive answer with RCODE_NOERROR and no RRs?? 
        headers[headerscount-1]->set_rcode(RFC1035_RCODE_NXDOMAIN); //RFC1536
        return 0;
    }

    if(anc){

        if((unsigned)k >= unparsedBufLen){
            return -1;
        }

        for(i = 0; i<anc; i++){
            k = uncompress(k);
            if(k == -1 || (unsigned)(k+9) >= unparsedBufLen){
                return -1;
            }
            u_int16_t type = (((u_int16_t)unparsedBuf[k])<<8) + unparsedBuf[k+1];
            if((type>16 || type == 0) && type != RFC1035_TYPE_AAAA){
                return -1;
            }
            u_int16_t rdlength = (((u_int16_t)unparsedBuf[k+8])<<8) + unparsedBuf[k+9];
            if( (unsigned)k+9+rdlength >= unparsedBufLen){
                return -1;
            }
            char * name = mempool->strdup(tmpBuf);
            u_int16_t ttype = type;
            if(type == RFC1035_TYPE_AAAA)
                ttype = 17;
            answers[ancount+i] = (this->*(parse_RR_funcs[ttype-1]))(rdlength, k+10);
            if(answers[ancount+i] == NULL){
                return -1;
            }
            answers[ancount+i]->name = name;
            answers[ancount+i]->ttl = (((u_int32_t)unparsedBuf[k+4])<<24) |
                (((u_int32_t)unparsedBuf[k+5])<<16) |
                (((u_int32_t)unparsedBuf[k+6])<<8) |
                unparsedBuf[k+7];

            answers[ancount+i]->type = type;
            answers[ancount+i]->header_id = headers[headerscount-1]->id;
            answers[ancount+i]->rr_class = (((u_int16_t)unparsedBuf[k+2])<<8) + unparsedBuf[k+3];

            allrrs[allrrsCur++] = answers[ancount+i];

            k+=(10+rdlength);
        }
        ancount += i;
    }

    if(nsc){

        if((unsigned)k >= unparsedBufLen){
            return -1;
        }

        for(i = 0; i<nsc; i++){
            k = uncompress(k);
            if(k == -1 || (unsigned)(k+9) >= unparsedBufLen){
                return -1;
            }
            u_int16_t type = (((u_int16_t)unparsedBuf[k])<<8) + unparsedBuf[k+1];
            if((type>16 || type == 0) && type != RFC1035_TYPE_AAAA){
                printf("type=%d, nsc=%d, i=%d, anc=%d\n", type, nsc, i, anc);
                return -1;
            }
            u_int16_t rdlength = (((u_int16_t)unparsedBuf[k+8])<<8) + unparsedBuf[k+9];
            if( (unsigned)k+9+rdlength >= unparsedBufLen){
                return -1;
            }
            char * name = mempool->strdup(tmpBuf);
            u_int16_t ttype = type;
            if(type == RFC1035_TYPE_AAAA)
                ttype = 17;
            authorities[nscount+i] = (this->*(parse_RR_funcs[ttype-1]))(rdlength, k+10);
            if(authorities[nscount+i] == NULL){
                return -1;
            }
            authorities[nscount+i]->name = name;
            authorities[nscount+i]->ttl = (((u_int32_t)unparsedBuf[k+4])<<24) |
                (((u_int32_t)unparsedBuf[k+5])<<16) |
                (((u_int32_t)unparsedBuf[k+6])<<8) |
                unparsedBuf[k+7];

            authorities[nscount+i]->type = type;
            authorities[nscount+i]->header_id = headers[headerscount-1]->id;
            authorities[nscount+i]->rr_class = (((u_int16_t)unparsedBuf[k+2])<<8) + unparsedBuf[k+3];

            allrrs[allrrsCur++] = authorities[nscount+i];

            k+=(10+rdlength);
        }
        nscount += i;
    }

    if(arc){

        if((unsigned)k >= unparsedBufLen){
            return -1;
        }

        for(i = 0; i<arc; i++){
            k = uncompress(k);
            if(k == -1 || (unsigned)(k+9) >= unparsedBufLen){
                return -1;
            }
            u_int16_t type = (((u_int16_t)unparsedBuf[k])<<8) + unparsedBuf[k+1];
            if((type>16 || type == 0) && type != RFC1035_TYPE_AAAA){
                return -1;
            }
            u_int16_t rdlength = (((u_int16_t)unparsedBuf[k+8])<<8) + unparsedBuf[k+9];
            if( (unsigned)k+9+rdlength >= unparsedBufLen){
                return -1;
            }
            char * name = mempool->strdup(tmpBuf);
            u_int16_t ttype = type;
            if(type == RFC1035_TYPE_AAAA)
                ttype = 17;
            additionals[arcount+i] = (this->*(parse_RR_funcs[ttype-1]))(rdlength, k+10);
            if(additionals[arcount+i] == NULL){
                return -1;
            }
            additionals[arcount+i]->name = name;
            additionals[arcount+i]->ttl = (((u_int32_t)unparsedBuf[k+4])<<24) |
                (((u_int32_t)unparsedBuf[k+5])<<16) |
                (((u_int32_t)unparsedBuf[k+6])<<8) |
                unparsedBuf[k+7];

            additionals[arcount+i]->type = type;
            additionals[arcount+i]->header_id = headers[headerscount-1]->id;
            additionals[arcount+i]->rr_class = (((u_int16_t)unparsedBuf[k+2])<<8) + unparsedBuf[k+3];

            allrrs[allrrsCur++] = additionals[arcount+i];

            k+=(10+rdlength);
        }
        arcount += i;
    }

    return 0;
}

int AsyncDNSReply::addparse(void * _buf, size_t _buflen)
{
    unparsedBuf = (u_int8_t*)_buf;
    unparsedBufLen = _buflen;

    RFC1035_MessageHeader ** _headers = (RFC1035_MessageHeader**)mempool->alloc((headerscount+1)*sizeof(RFC1035_MessageHeader*));
    for(size_t i = 0; i < headerscount; i++){
        _headers[i] = headers[i];
    }
    headers = _headers;
    headerscount++;

    headers[headerscount-1] = (RFC1035_MessageHeader*)mempool->alloc(sizeof(RFC1035_MessageHeader));
    memcpy(headers[headerscount-1], unparsedBuf, sizeof(RFC1035_MessageHeader));

    int k = sizeof(RFC1035_MessageHeader);

    if((unsigned)k > unparsedBufLen){
        return -1;
    }

    /*
    for(size_t i = 0; i<unparsedBufLen; i++){
        printf("%02x ", unparsedBuf[i]);
    }
    printf("\n");
    */

    if(!tmpBuf)
        tmpBuf = (char*)mempool->alloc(RFC1035_MAX_NAMESIZE+1);

    u_int16_t qdc = headers[headerscount-1]->get_qdcount();
    u_int16_t anc = headers[headerscount-1]->get_ancount();
    u_int16_t nsc = headers[headerscount-1]->get_nscount();
    u_int16_t arc = headers[headerscount-1]->get_arcount();

    RFC1035_RR ** _allrrs = (RFC1035_RR**)mempool->alloc((anc+nsc+arc+ancount+nscount+arcount)*sizeof(RFC1035_RR*));
    for(size_t i = 0; i < (unsigned)(arcount+nscount+ancount); i++){
        _allrrs[i] = allrrs[i];
    }
    allrrs = _allrrs;

    RFC1035_MessageQuestion ** _questions = (RFC1035_MessageQuestion**)mempool->alloc((qdcount+qdc)*(sizeof(RFC1035_MessageQuestion*)));
    for(size_t i = 0; i < qdcount; i++){
        _questions[i] = questions[i];
    }
    questions = _questions;

    RFC1035_RR ** _answers = (RFC1035_RR**)mempool->alloc((ancount+anc)*(sizeof(RFC1035_RR*)));
    for(size_t i = 0; i < ancount; i++){
        _answers[i] = answers[i];
    }
    answers = _answers;

    RFC1035_RR ** _authorities = (RFC1035_RR**)mempool->alloc((nscount+nsc)*(sizeof(RFC1035_RR*)));
    for(size_t i = 0; i < nscount; i++){
        _authorities[i] = authorities[i];
    }
    authorities = _authorities;

    RFC1035_RR ** _additionals = (RFC1035_RR**)mempool->alloc((arcount+arc)*(sizeof(RFC1035_RR*)));
    for(size_t i = 0; i < arcount; i++){
        _additionals[i] = additionals[i];
    }
    additionals = _additionals;

    return parseinternal();
}

const RFC1035_MessageHeader * AsyncDNSReply::getHeader(size_t i) const
{
    if(headers != NULL && i < headerscount ){
        return headers[i];
    }
    return NULL;
}

const RFC1035_MessageQuestion * AsyncDNSReply::getQuestion(size_t i) const
{
    if(headers != NULL && i < qdcount ){
        return questions[i];
    }
    return NULL;
}

const RFC1035_RR * AsyncDNSReply::getAnswer(size_t i) const
{
    if(headers != NULL && i < ancount){
        return answers[i];
    }
    return NULL;
}

const RFC1035_RR * AsyncDNSReply::getAuthority(size_t i) const
{
    if(headers != NULL && i < nscount ){
        return authorities[i];
    }
    return NULL;
}

const RFC1035_RR * AsyncDNSReply::getAdditional(size_t i) const
{
    if(headers != NULL && i < arcount ){
        return additionals[i];
    }
    return NULL;
}

const RFC1035_RR * AsyncDNSReply::getRR(size_t i) const
{
    if(headers != NULL && i < (unsigned)(arcount+nscount+ancount) ){
        return allrrs[i];
    }
    return NULL;
}

const RFC1035_RR * AsyncDNSReply::getLastRRByType(u_int8_t type) const
{
    if(type == RFC1035_TYPE_AAAA)
        type = 17;
    if(type < 1 || type > 17){
        return NULL;
    }
    for(int i = (arcount+nscount+ancount-1); i >= 0; i--){
        if(allrrs[i]->type == type){
            return allrrs[i];
        }
    }
    return NULL;
}

void AsyncDNSReply::cleanup()
{
    qdcount = 0;
    ancount = 0;
    nscount = 0;
    arcount = 0;
    allrrsCur = 0;
    headerscount = 0;

    questions = NULL;
    answers = NULL;
    authorities = NULL;
    additionals = NULL;
    allrrs = NULL;
    tmpBuf = NULL;
    headers = NULL;

    for(size_t i = 0; i < 16; i++){
        prevRR[i] = NULL;
    }
}

RFC1035_RR * AsyncDNSReply::parse_A_RR(u_int16_t rdlength, u_int16_t from)
{
    if(from > (unparsedBufLen-4)){
        return NULL;
    }

    RFC1035_A_RR * a_rr = (RFC1035_A_RR*)mempool->alloc(sizeof(RFC1035_A_RR));

    a_rr->prevRR = prevRR[0]; prevRR[0] = a_rr;
    a_rr->address.s_addr = (((u_int32_t)unparsedBuf[from+3])<<24) |
        (((u_int32_t)unparsedBuf[from+2])<<16) |
        (((u_int32_t)unparsedBuf[from+1])<<8) |
        unparsedBuf[from];

    return a_rr;
}

RFC1035_RR * AsyncDNSReply::parse_NS_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_NS_RR * ns_rr = (RFC1035_NS_RR*)mempool->alloc(sizeof(RFC1035_NS_RR));

    ns_rr->prevRR = prevRR[1]; prevRR[1] = ns_rr;
    ns_rr->nsdname = mempool->strdup(tmpBuf);

    return ns_rr;
}

RFC1035_RR * AsyncDNSReply::parse_MD_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_MD_RR * md_rr = (RFC1035_MD_RR*)mempool->alloc(sizeof(RFC1035_MD_RR));

    md_rr->prevRR = prevRR[2]; prevRR[2] = md_rr;
    md_rr->madname = mempool->strdup(tmpBuf);

    return NULL;
}

RFC1035_RR * AsyncDNSReply::parse_MF_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_MF_RR * mf_rr = (RFC1035_MF_RR*)mempool->alloc(sizeof(RFC1035_MF_RR));

    mf_rr->prevRR = prevRR[3]; prevRR[3] = mf_rr;
    mf_rr->madname = mempool->strdup(tmpBuf);

    return mf_rr;
}

RFC1035_RR * AsyncDNSReply::parse_CNAME_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_CNAME_RR * cname_rr = (RFC1035_CNAME_RR*)mempool->alloc(sizeof(RFC1035_CNAME_RR));

    cname_rr->prevRR = prevRR[4]; prevRR[4] = cname_rr;
    cname_rr->cname = mempool->strdup(tmpBuf);
    cname_rr->resolver_checked = 0;

    return cname_rr;
}

RFC1035_RR * AsyncDNSReply::parse_SOA_RR(u_int16_t rdlength, u_int16_t from)
{
    int k = 0;
    if( (k = uncompress(from)) == -1){
        return NULL;
    }
    char * mname = mempool->strdup(tmpBuf);

    if( (k = uncompress(k)) == -1){
        return NULL;
    }
    char * rname = mempool->strdup(tmpBuf);

    if( (unsigned)(k+20) > unparsedBufLen ){
        return NULL;
    }

    u_int32_t serial = (((u_int32_t)unparsedBuf[k])<<24) |
        (((u_int32_t)unparsedBuf[k+1])<<16) |
        (((u_int32_t)unparsedBuf[k+2])<<8) |
        unparsedBuf[k+3];

    u_int32_t refresh = (((u_int32_t)unparsedBuf[k+4])<<24) |
        (((u_int32_t)unparsedBuf[k+5])<<16) |
        (((u_int32_t)unparsedBuf[k+6])<<8) |
        unparsedBuf[k+7];

    u_int32_t retry = (((u_int32_t)unparsedBuf[k+8])<<24) |
        (((u_int32_t)unparsedBuf[k+9])<<16) |
        (((u_int32_t)unparsedBuf[k+10])<<8) |
        unparsedBuf[k+11];

    u_int32_t expire = (((u_int32_t)unparsedBuf[k+12])<<24) |
        (((u_int32_t)unparsedBuf[k+13])<<16) |
        (((u_int32_t)unparsedBuf[k+14])<<8) |
        unparsedBuf[k+15];

    u_int32_t minimum = (((u_int32_t)unparsedBuf[k+16])<<24) |
        (((u_int32_t)unparsedBuf[k+17])<<16) |
        (((u_int32_t)unparsedBuf[k+18])<<8) |
        unparsedBuf[k+19];

    RFC1035_SOA_RR * soa_rr = (RFC1035_SOA_RR*)mempool->alloc(sizeof(RFC1035_SOA_RR));

    soa_rr->prevRR = prevRR[5]; prevRR[5] = soa_rr;
    soa_rr->mname = mname;
    soa_rr->rname = rname;
    soa_rr->serial = serial;
    soa_rr->refresh = refresh;
    soa_rr->retry = retry;
    soa_rr->expire = expire;
    soa_rr->minimum = minimum;

    return soa_rr;
}

RFC1035_RR * AsyncDNSReply::parse_MB_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_MB_RR * mb_rr = (RFC1035_MB_RR*)mempool->alloc(sizeof(RFC1035_MB_RR));

    mb_rr->prevRR = prevRR[6]; prevRR[6] = mb_rr;
    mb_rr->madname = mempool->strdup(tmpBuf);

    return mb_rr;
}

RFC1035_RR * AsyncDNSReply::parse_MG_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_MG_RR * mg_rr = (RFC1035_MG_RR*)mempool->alloc(sizeof(RFC1035_MG_RR));

    mg_rr->prevRR = prevRR[7]; prevRR[7] = mg_rr;
    mg_rr->mgmname = mempool->strdup(tmpBuf);

    return mg_rr;
}

RFC1035_RR * AsyncDNSReply::parse_MR_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_MR_RR * mr_rr = (RFC1035_MR_RR*)mempool->alloc(sizeof(RFC1035_MR_RR));

    mr_rr->prevRR = prevRR[8]; prevRR[8] = mr_rr;
    mr_rr->newname = mempool->strdup(tmpBuf);

    return mr_rr;
}

RFC1035_RR * AsyncDNSReply::parse_NULL_RR(u_int16_t rdlength, u_int16_t from)
{
    RFC1035_NULL_RR * null_rr = (RFC1035_NULL_RR*)mempool->alloc(sizeof(RFC1035_NULL_RR));

    null_rr->prevRR = prevRR[9]; prevRR[9] = null_rr;
    null_rr->anything = NULL; //FIXME!

    return null_rr;
}

RFC1035_RR * AsyncDNSReply::parse_WKS_RR(u_int16_t rdlength, u_int16_t from)
{
    RFC1035_WKS_RR * wks_rr = (RFC1035_WKS_RR*)mempool->alloc(sizeof(RFC1035_WKS_RR));

    wks_rr->prevRR = prevRR[10]; prevRR[10] = wks_rr;

    return wks_rr; //FIXME!
}

RFC1035_RR * AsyncDNSReply::parse_PTR_RR(u_int16_t rdlength, u_int16_t from)
{
    if(uncompress(from) == -1){
        return NULL;
    }

    RFC1035_PTR_RR * ptr_rr = (RFC1035_PTR_RR*)mempool->alloc(sizeof(RFC1035_PTR_RR));

    ptr_rr->prevRR = prevRR[11]; prevRR[11] = ptr_rr;
    ptr_rr->ptrdname = mempool->strdup(tmpBuf);

    return ptr_rr;
}

RFC1035_RR * AsyncDNSReply::parse_HINFO_RR(u_int16_t rdlength, u_int16_t from)
{
    RFC1035_HINFO_RR * hinfo_rr = (RFC1035_HINFO_RR*)mempool->alloc(sizeof(RFC1035_HINFO_RR));

    hinfo_rr->prevRR = prevRR[12]; prevRR[12] = hinfo_rr;
    hinfo_rr->cpu = NULL;
    hinfo_rr->os = NULL;
    
    int count = chrstrCount(rdlength, from);
    if(count == -1 || count > 2){
        return NULL;
    }    
    
    int f = from;
    if(count > 0){
        if((f = chrstrParse(f)) == -1){
            return NULL;
        }    
        hinfo_rr->cpu = mempool->strdup(tmpBuf);
    }
    
    if(count == 2){
        if((f = chrstrParse(f)) == -1){
            return NULL;
        }
        hinfo_rr->os = mempool->strdup(tmpBuf);
    }
    
    return hinfo_rr;
}

RFC1035_RR * AsyncDNSReply::parse_MINFO_RR(u_int16_t rdlength, u_int16_t from)
{
    int k = 0;
    if( (k = uncompress(from)) == -1){
        return NULL;
    }
    char * rmailbx = mempool->strdup(tmpBuf);

    if( (k = uncompress(k)) == -1){
        return NULL;
    }

    RFC1035_MINFO_RR * minfo_rr = (RFC1035_MINFO_RR*)mempool->alloc(sizeof(RFC1035_MINFO_RR));

    minfo_rr->prevRR = prevRR[13]; prevRR[13] = minfo_rr;
    minfo_rr->rmailbx = rmailbx;
    minfo_rr->emailbx = mempool->strdup(tmpBuf);

    return minfo_rr;
}

RFC1035_RR * AsyncDNSReply::parse_MX_RR(u_int16_t rdlength, u_int16_t from)
{
    if(from > (unparsedBufLen-4)){
        return NULL;
    }

    u_int16_t preference = (((u_int16_t)unparsedBuf[from])<<8) + unparsedBuf[from+1];
    
    if(uncompress(from+2) == -1){
        return NULL;
    }

    RFC1035_MX_RR * mx_rr = (RFC1035_MX_RR*)mempool->alloc(sizeof(RFC1035_MX_RR));

    mx_rr->prevRR = prevRR[14]; prevRR[14] = mx_rr;
    mx_rr->preference = preference;
    mx_rr->exchange = mempool->strdup(tmpBuf);
    mx_rr->resolver_checked = 0;

    return mx_rr;
}

RFC1035_RR * AsyncDNSReply::parse_TXT_RR(u_int16_t rdlength, u_int16_t from)
{
    RFC1035_TXT_RR * txt_rr = (RFC1035_TXT_RR*)mempool->alloc(sizeof(RFC1035_TXT_RR));

    txt_rr->prevRR = prevRR[15]; prevRR[15] = txt_rr;
    
    int count = chrstrCount(rdlength, from);
    if(count == -1){
        return NULL;
    }
    
    txt_rr->txt_data = (char**)mempool->alloc((count+1)*sizeof(char*));
    
    int f = from;
    for(int i = 0; i<count; i++){
        if((f = chrstrParse(f)) == -1){
            return NULL;
        }
        txt_rr->txt_data[i] = mempool->strdup(tmpBuf);
    }    
    txt_rr->txt_data[count] = NULL;

    return txt_rr;
}

RFC1035_RR * AsyncDNSReply::parse_AAAA_RR(u_int16_t rdlength, u_int16_t from)
{
    if(from >= (unparsedBufLen-15)){
        return NULL;
    }
    
    RFC1035_AAAA_RR * aaaa_rr = (RFC1035_AAAA_RR*)mempool->alloc(sizeof(RFC1035_AAAA_RR));

    memcpy(&aaaa_rr->address, &unparsedBuf[from], sizeof(aaaa_rr->address));

    aaaa_rr->prevRR = prevRR[16]; prevRR[16] = aaaa_rr;

    return aaaa_rr;
}

