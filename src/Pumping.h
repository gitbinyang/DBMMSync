#ifndef PUMPING_H
#define PUMPING_H
#include <poll.h>
#include <map>
#include "Monitor.h"

#define MAX_MSGLEN     65535

typedef enum{
    STATE_CLOSED=1,
    STATE_INIT,
    STATE_ADMINUP,
    //STATE_SPRDUP,
    STATE_LOOP,

}STATE;


class AppState{
public:
    static int get_state(){ return (int)s;}
    static void set_state(STATE t){s = t;}    
private:    
    static STATE s;
};

class Pumping{
public:
    Pumping();    
    ~Pumping();     
    int add_monitor(Monitor* m);
    int del_monitor(Monitor* m);
    int del_monitor(int dfd);
    int clear_monitors();
    void run();
    Monitor* get_monitor_bytype(int type);        
private:    
        
    void poll_fds();
    Monitor* get_monitor_byslot(int slot);
    
    struct pollfd fds[MAX_SOCK];    
    std::map<int, Monitor*>map_slot_monitor;    
};

#endif

