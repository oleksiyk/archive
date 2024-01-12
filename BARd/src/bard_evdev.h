#ifndef BARD_EVDEV_H
#define BARD_EVDEV_H

#include "config.h"
#include "log.h"
#include "bard_sys.h"
#include "bard_signal.h"
#include "bard_config.h"

class BARdEvdev
{
private:
    const Log * log;
    int fd;
    struct input_event events[64];

public:
    BARdEvdev(const Log * _log, int _fd);
    virtual ~BARdEvdev();

public:
    int logInfo() const;
    int readData(char * buf, size_t szbuf);
};

#endif
