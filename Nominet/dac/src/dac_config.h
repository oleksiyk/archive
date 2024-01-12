/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_CONFIGURATION_H
#define DAC_CONFIGURATION_H

#include "decl.h"
#include "dotconfpp.h"

#include <list>

class DacConfig : public DOTCONFDocument
{
private:
    virtual void error(int lineNum, const char * fileName, const char * fmt, ...);
    const Log * log;
    float getMin(const char * v);
    float getMax(const char * v);
    int parseThresholds();

    int parseNetworkAddress(const char *str, u_int32_t *addr, u_int32_t *mask);
    int parseHostLine(const DOTCONFDocumentNode * node, u_int32_t *addr);
    int parseAccessLines();

public:

    struct ipms {
        u_int32_t addr;
        u_int32_t mask;
    };

    std::list<ipms> ipAllowList;

public:
    int checkConfig();

    float latency_thresholds[5][3];
    float jitter_thresholds[5][3];
    float loss_thresholds[5][3];

    DacConfig(const Log * _log, DOTCONFDocument::CaseSensitive caseSensitivity)
        :DOTCONFDocument(caseSensitivity), log(_log) {};
};


#endif
