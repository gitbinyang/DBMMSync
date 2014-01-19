#ifndef CLNTSOCKMONITOR_H
#define CLNTSOCKMONITOR_H
#include "Monitor.h"

class ClntSockMonitor : public Monitor{
public:
    ClntSockMonitor(int clnt_fd);
    virtual ~ClntSockMonitor();
    virtual int init(Pumping* pump);
    virtual int get_fd();
    virtual int get_monitor_type();
    virtual int IO_event_callback(int fd);
private:
    int ClntSock_fd;
};

#endif

