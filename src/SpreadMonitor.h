#ifndef SPREADMONITOR_H
#define SPREADMONITOR_H
#include "sp.h"

#include "Monitor.h"
#include "SQLloader.h"

#define MAXNSPGROUP 10

typedef enum {
    SP_UNKNOWN_MESS=0,
    SP_DATA,
    SP_MEMBER_JOIN,
    SP_MEMBER_LEAVE,
    SP_MEMBER_DISC,
}SP_event_t;


class SpreadMonitor : public Monitor{
public:
    SpreadMonitor(SQLloader* loader);
    virtual ~SpreadMonitor();
    virtual int init(Pumping* pump);
    virtual int get_fd(); 
    virtual int get_monitor_type();
    virtual int IO_event_callback(int fd);
    int SP_mcast(unsigned char* inbuff, int size);
    int get_SP_group(unsigned char* group_list);    
protected:
    int SP_conect();    
    int SP_recv_timeo(mailbox mbox, char sender[MAX_GROUP_NAME],
        int16 *msg_type, int buf_size, char *inbuff, SP_event_t* sp_event,  int* num_group,  int timeout);     
    int process_SP_mess(unsigned char* buff, int size);
    int SP_members_change(SP_event_t event);
    void SP_quit(mailbox box, char* group_name);
private:
    mailbox mbox;
    char groups[MAXNSPGROUP][MAX_GROUP_NAME];
    int num_group;
    char self_group[32]; //private spread group 
    SQLloader* sqlloader;
};
#endif

