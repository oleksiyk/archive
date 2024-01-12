/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_HISTORY_H
#define PROBED_HISTORY_H

#include "decl.h"

class ProbedHistory
{
private:
    FILE * fp;

public:
    ProbedHistory();
    virtual ~ProbedHistory();

    int vlog(const char * msg, va_list ap)const;
    int log(const char * msg, ...)const;

    int initialize(const char * fileName);
};

#endif
