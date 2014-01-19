#ifndef MONITOR_H
#define MONITOR_H
#define MAX_MSGLEN 65535
#define MAX_SOCK       10
#include <fcntl.h>
#include <string.h>
#include "Utils.h"

//monitor PG log, spread incoming message and admin socket connect
typedef enum{
    PGFILE=1,
    SPREAD,
    SRVSOCK,   //admin socket server
    CLNTSOCK,  //admin socket client
}FD_TYPE;

class Pumping;
class Monitor{
public:
    Monitor(){O_pump = NULL;}
    virtual ~Monitor(){}
    virtual int init(Pumping* pump){
    	O_pump = pump;
    	return 1;
    }
    virtual int get_fd()=0;
    virtual int get_monitor_type()=0;

    //this monitor will be removed if return value < 0
    virtual int IO_event_callback(int fd){
        memset(inbuff, 0x0,MAX_MSGLEN);
        return 1;
    }
    
protected:
    Pumping* O_pump;
    unsigned char inbuff[MAX_MSGLEN];
    unsigned char outbuff[MAX_MSGLEN];      

    int set_IO_unblocking(int fd){
        int flags=fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) == -1) {
          log(LOG_CRIT, "set IO unblocking failed: %s\n", strerror(errno));
          return -1;
        }
        return 1;
    }
};

#endif

