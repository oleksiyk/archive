#ifndef SHOTGUND_CONFIGURATION_H
#define SHOTGUND_CONFIGURATION_H

#include "log.h"
#include "dotconfpp.h"

class ShotgundConfig : public DOTCONFDocument
{
private:
    virtual void error(int lineNum, const char * fileName, const char * fmt, ...);
    const Log * log;
public:
    int async_query;
    int checkConfig();
    ShotgundConfig(const Log * _log, DOTCONFDocument::CaseSensitive caseSensitivity)
        :DOTCONFDocument(caseSensitivity), log(_log), async_query(1){};
};


#endif
