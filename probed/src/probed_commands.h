/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_COMMANDS_H
#define PROBED_COMMANDS_H

#include "decl.h"

class ProbedCommands
{
private:
    const Log * log;
    ProbedSSLTransport * tr;
    const char * peerName;

    ProbedPingTask task;

    typedef int (ProbedCommands::*tCommandFunc)(const char * buf);

    static struct tCP {
        const char * command;
        tCommandFunc commandFunc;
    } cp[];

    //int command_HELO(const char * buf);
    int command_PACKETS(const char * buf);
    int command_QUERY(const char * buf);
    int command_VERBOSE(const char * buf);
    int command_OPTIONS(const char * buf);
    int command_BYE(const char * buf);

    int unknownCommand(const char * buf);

public:
    ProbedCommands(const Log * _log, ProbedSSLTransport * _tr, ProbedThread * _thread);
    virtual ~ProbedCommands();

    int process(const char * buf);
};

#endif
