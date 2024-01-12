/*  Copyright (C) 2003 
*   Alexander Biehl,
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

#include <rfc1035.h>

#include <getopt.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

/* forward declaration: will print RR to stdout */
void dump(const RFC1035_RR * answer);
void resolv_cname(const AsyncDNSReply * reply, const char * cname);

bool debug = false;

int main(int argc, char** argv)
{
    int c;
    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"debug", no_argument, NULL, 'd'},
    };

    while ((c = getopt_long (argc, argv, "hd", long_options, NULL)) != -1) {
        switch(c) {
            case 'h':
            {
                printf("syntax: %s [-h] [-d] <(sub-)domain)>\n", argv[0]);
                exit(0);
            }
            case 'd':
            {
                debug = true;
            }
        }
    }

    char *bp;
    if (optind < argc) {
        bp = argv[optind];
    }
    else {
        printf("too few parameters: (sub-)domain missed\n");
        return 1;
    }

    /* enable core dump */
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    /* create resolver object */
    AsyncDNSResolver * resolver = new AsyncDNSResolver();

    /* initialize resolver */
    printf("initialize resolver: ");
    if(resolver->initialize() == -1){
        printf("FAILED\n");
        return -1;
    }
    else {
        printf("OK\n");
    }

    /* set query type (recursive, status/normal/inverse query) */
    printf("set query typ: ");
    if(resolver->prepareQuery(RFC1035_OPCODE_QUERY, RFC1035_RESOLVE_RECURSIVE) == -1){
        printf("FAILED\n");
        return -1;
    }
    else {
        printf("OK\n");
    }

    /* set query ( query name, resource record type, class) */
    printf("set query: ");
    if(resolver->addQuery(bp, RFC1035_TYPE_MX, RFC1035_CLASS_IN) == -1){
        printf("FAILED\n");
        return -1;
    }
    else {
        printf("OK\n");
    }

    /* wait for query completion */
    /* 
        resolveQueries() returns EAGAIN if there is no data currently avail.
        
        You can use resolver->wait(msec) to wait for data or you can poll()
        fdset returned by resolver->getPollfd() in your own loop.
        
        Timeouts are checked by resolver->timedout() function. it returns -1
        if query timedout.
    */
    printf("resolveQueries: ");
    while(resolver->resolveQueries() == EAGAIN){
        if(resolver->wait(10000) != 0){
            if(resolver->timedout() == -1){
                break;
            }
        }
    }
    printf("OK\n");


    /* get reply */
    printf("get reply: ");
    const AsyncDNSReply * reply = resolver->getReply();

    
    /* get all info from reply */
    if(reply){
        printf("OK\n\n");
        

        // MX resource record
        printf("\n-----------------------------------\nAll MX's:\n");
        const RFC1035_RR * answer = NULL;
        answer = reply->getLastRRByType(RFC1035_TYPE_MX);
        while(answer){
            if(answer->type == RFC1035_TYPE_MX){
                printf("\tpreference=%d, exchange=%s\n", ((RFC1035_MX_RR*)answer)->preference, ((RFC1035_MX_RR*)answer)->exchange);

                // get ip for mx
                const RFC1035_RR * answer_cname = NULL;
                answer_cname = reply->getLastRRByType(RFC1035_TYPE_CNAME);
                while(answer_cname) {
                    if( (answer_cname->type == RFC1035_TYPE_CNAME) && (!strcasecmp(answer_cname->name, ((RFC1035_MX_RR*)answer)->exchange)) ){
                        printf("\t\tcname=%s\n", ((RFC1035_CNAME_RR*)answer_cname)->cname);
                        resolv_cname(reply, ((RFC1035_CNAME_RR*)answer_cname)->cname);
                    }
                    answer_cname = answer_cname->prevRR;
                }

                const RFC1035_RR * answer_ip = NULL;
                answer_ip = reply->getLastRRByType(RFC1035_TYPE_A);
                while(answer_ip) {
                    if( (answer_ip->type == RFC1035_TYPE_A) && (!strcasecmp(answer_ip->name, ((RFC1035_MX_RR*)answer)->exchange)) ){
                        printf("\t\taddress=%s\n", inet_ntoa(((RFC1035_A_RR*)answer_ip)->address));
                    }
                    answer_ip = answer_ip->prevRR;
                }
                /*
                answer_ip = reply->getLastRRByType(RFC1035_TYPE_AAAA);
                while(answer_ip) {
                    if( (answer_ip->type == RFC1035_TYPE_AAAA) && (!strcasecmp(answer_ip->name, ((RFC1035_MX_RR*)answer)->exchange)) ){
                        char nsaddr[RFC1035_INET_ADDRSTRLEN];
                        if(inet_ntop(RFC1035_AF_INET, &((RFC1035_AAAA_RR*)answer)->address, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                            nsaddr[0] = 0;
                        }
                        printf("\t\tAAAA address=%s\n", nsaddr);
                    }
                    answer_ip = answer_ip->prevRR;
                }
                */
            }
            answer = answer->prevRR;
        }
        printf("\n");

        if (debug) {
            printf("\n-----------------------------------\ndebug information:\n");
            const RFC1035_MessageHeader * header = NULL;
            size_t i = 0;
            printf("Headers:\n");
            // there can be several headers ( resolver can make its own queries )
            while((header = reply->getHeader(i++)) != NULL){
                printf("\tID=%d, RCODE=%d, %s\n", header->id, header->get_rcode(),
                    header->get_aa()?"Authoritative answer":"Not authoritative answer");            
            }
            printf("\n");

            // questions
            const RFC1035_MessageQuestion * question = reply->getQuestion();
            printf("Questions:\n");
            i = 0;
            while((question = reply->getQuestion(i++)) != NULL){
                printf("\tqname=%s, qtype=%d, qclass=%d, header_id=%d\n", question->qname, question->qtype, question->qclass, question->header_id);
            }
            printf("\n");
        
        
            // answers
            printf("Answers:\n");
            i = 0;
            while((answer = reply->getAnswer(i++)) != NULL){
                dump(answer);
            }
            printf("Authorities:\n");
            i = 0;
            while((answer = reply->getAuthority(i++)) != NULL){
                dump(answer);
            }
            printf("Additional:\n");
            i = 0;
            while((answer = reply->getAdditional(i++)) != NULL){
                dump(answer);       
            }
            printf("\n");
        
            // all resource records
            printf("\n-----------------------------------\nAll RR's:\n");
            i = 0;
            while((answer = reply->getRR(i++)) != NULL){
                dump(answer);
            }
            printf("\n");
        }

    }
    else {
        printf("FAILED\n\n");
    }

    resolver->cleanup(); //must be called after each call to resolveQueries

    delete resolver;

    return 0;
}

void dump(const RFC1035_RR * answer)
{
    if(answer->type == RFC1035_TYPE_MX){
        printf("\tMX: name=%s, preference=%d, exchange=%s, header_id=%d\n", answer->name, ((RFC1035_MX_RR*)answer)->preference, ((RFC1035_MX_RR*)answer)->exchange, answer->header_id);
        return;
    }
    if(answer->type == RFC1035_TYPE_CNAME){
        printf("\tCNAME: name=%s, cname=%s, header_id=%d\n", answer->name, ((RFC1035_CNAME_RR*)answer)->cname, answer->header_id);
        return;
    }
    if(answer->type == RFC1035_TYPE_A){
        printf("\tA: name=%s, address=%s, header_id=%d\n", answer->name, inet_ntoa(((RFC1035_A_RR*)answer)->address), answer->header_id);
        return;
    }
    if(answer->type == RFC1035_TYPE_SOA){
        printf("\tSOA: name=%s, mname=%s, rname=%s, serial=%d, refresh=%d, retry=%d, expire=%d, minimum=%d, header_id=%d\n", 
            answer->name,
            ((RFC1035_SOA_RR*)answer)->mname,
            ((RFC1035_SOA_RR*)answer)->rname,
            ((RFC1035_SOA_RR*)answer)->serial,
            ((RFC1035_SOA_RR*)answer)->refresh,
            ((RFC1035_SOA_RR*)answer)->retry,
            ((RFC1035_SOA_RR*)answer)->expire,
            ((RFC1035_SOA_RR*)answer)->minimum, answer->header_id);
        return;
    }
    if(answer->type == RFC1035_TYPE_NS){
        printf("\tNS: name=%s, nsdname=%s, header_id=%d\n", answer->name, ((RFC1035_NS_RR*)answer)->nsdname, answer->header_id);
        return;
    }
    if(answer->type == RFC1035_TYPE_PTR){
        printf("\tPTR: name=%s, ptrdname=%s, header_id=%d\n", answer->name, ((RFC1035_PTR_RR*)answer)->ptrdname, answer->header_id);
        return;
    }
    if(answer->type == RFC1035_TYPE_TXT){
        printf("\tTXT: name=%s, text=", answer->name);
        int i = 0;
        char * txt = ((RFC1035_TXT_RR*)answer)->txt_data[i];
        while(txt){
            printf("'%s' ", txt);
            i++;
            txt = ((RFC1035_TXT_RR*)answer)->txt_data[i];
        }
        printf("\n");
        return;
    }
    if(answer->type == RFC1035_TYPE_HINFO){
        printf("\tHINFO: name=%s", answer->name);
        char * cpu = ((RFC1035_HINFO_RR*)answer)->cpu;
        if(cpu){
            printf(", cpu='%s'", cpu);
        }
        char * os = ((RFC1035_HINFO_RR*)answer)->os;
        if(os){
            printf(", os='%s'", os);
        }
        printf("\n");
        return;
    }
    printf("UNPARSED: answer->type=%d, header_id=%d\n", answer->type, answer->header_id);
}


void resolv_cname(const AsyncDNSReply * reply, const char * cname)
{
    const RFC1035_RR * answer_cname = NULL;
    answer_cname = reply->getLastRRByType(RFC1035_TYPE_CNAME);
    while(answer_cname) {
        if( (answer_cname->type == RFC1035_TYPE_CNAME) && (!strcasecmp(answer_cname->name, cname )) ) {
            printf("\t\tcname=%s\n", ((RFC1035_CNAME_RR*)answer_cname)->cname);
            resolv_cname(reply, ((RFC1035_CNAME_RR*)answer_cname)->cname);
        }
        answer_cname = answer_cname->prevRR;
    }

    const RFC1035_RR * answer_ip = NULL;
    answer_ip = reply->getLastRRByType(RFC1035_TYPE_A);
    while(answer_ip) {
        if( (answer_ip->type == RFC1035_TYPE_A) && (!strcasecmp(answer_ip->name, cname) ) ) {
            printf("\t\taddress=%s\n", inet_ntoa(((RFC1035_A_RR*)answer_ip)->address));
        }
        answer_ip = answer_ip->prevRR;
    }
}

