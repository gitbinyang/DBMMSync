#ifndef SRVSOCKMONITOR_H
#define SRVSOCKMONITOR_H
#include "Monitor.h"

class SrvSockMonitor : public Monitor{
public:
    SrvSockMonitor();
    virtual ~SrvSockMonitor();
    virtual int init(Pumping* pump);
    virtual int get_fd();
    virtual int get_monitor_type();
    virtual int IO_event_callback(int fd);
private:
    int SrvSock_fd;
};

#endif
