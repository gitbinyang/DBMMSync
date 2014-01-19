#include <assert.h>
#include <sys/time.h>
#include <errno.h>
//#include <map>

#include "Utils.h"
#include "Pumping.h"


#define TIMEOUT                5000

STATE AppState::s;
Pumping::Pumping(){
	for(int slot=0; slot < MAX_SOCK; ++slot){
		fds[slot].fd =-1;
	}
}

Pumping::~Pumping(){
	int c = clear_monitors();		
	log(LOG_DEBUG, "%d monitors are cleared", c);
}

int Pumping::clear_monitors(){
	std::map<int, Monitor*>::iterator it;
	int count=0;
	for(int slot=0; slot < MAX_SOCK; ++slot){
		if(fds[slot].fd >= 0){		
			fds[slot].fd =-1;
			it = map_slot_monitor.find(slot);
			Monitor* mt = (*it).second;
			//log(LOG_DEBUG, "monitor type %d deleted", mt->get_monitor_type());
			map_slot_monitor.erase(it);
			delete mt;
			count++;
		}
	}
	return count;
}


int Pumping::add_monitor(Monitor* mt){
	assert(mt != NULL);
	int slot;
	for(slot=0; slot < MAX_SOCK; ++slot){
		if(fds[slot].fd < 0)
			break;
	}
	assert(slot < (MAX_SOCK-1));
	fds[slot].fd	   = mt->get_fd();
	fds[slot].events = POLLIN;
	map_slot_monitor.insert(std::pair<int, Monitor*>(slot,mt));
	
	return 1;	
	
}


int Pumping::del_monitor(int dfd){
	std::map<int, Monitor*>::iterator it;
	for(int slot = 0; slot< MAX_SOCK; ++slot){
		if(fds[slot].fd == dfd){
			fds[slot].fd = -1;
			it = map_slot_monitor.find(slot);
			Monitor* mt = (*it).second;
			map_slot_monitor.erase(it);
			delete mt;
			log(LOG_DEBUG, "delete monitor associated with fd:%d", dfd);
			return 1;
		}
	}
	log(LOG_ERR, "unable to find the monitor associated with fd:%d", dfd);
	return -1;	
}


int Pumping::del_monitor(Monitor* mt){
	return del_monitor(mt->get_fd());
}


void Pumping::run(){
    //struct timeval tstart;
    //int size, i;
    
	int timeout = 5000;
    while(AppState::get_state() == STATE_LOOP){
        int res = poll(fds, MAX_SOCK, timeout);
		
        /* timed out */
        if (res == 0) {
			log(LOG_DEBUG, "pump_event timeout");
            
        }else if ((res < 0) && (errno != EINTR)) {
            log(LOG_DEBUG, "SPU poll error");
            return ; 
        }else{
        	log(LOG_DEBUG, "io event is here");
			for(int slot=0; slot < MAX_SOCK; ++slot){
				if(fds[slot].revents & POLLIN){
					Monitor* mt = get_monitor_byslot(slot);
					assert(mt != NULL);
					int ret = mt->IO_event_callback(fds[slot].fd);
					if(ret < 0){
						del_monitor(mt);
					}
				}
			}
        }
	} ;

}


Monitor* Pumping::get_monitor_byslot(int slot){
	std::map<int, Monitor*>::iterator it;
	Monitor* mt = NULL;
	for(it = map_slot_monitor.begin(); it != map_slot_monitor.end(); ++it){
		if( (*it).first == slot){
			mt = (*it).second;
			log(LOG_DEBUG, "event slot:%d, monitor type :%d", slot, mt->get_monitor_type()); 
			return mt;
		}
	}
	return NULL;
}


//notice: it only search the first matched monitor
Monitor* Pumping::get_monitor_bytype(int type){
	std::map<int, Monitor*>::iterator it;
	Monitor* mt = NULL;
	for(it = map_slot_monitor.begin(); it != map_slot_monitor.end(); ++it){
		mt = (*it).second;
		if( mt->get_monitor_type()== type){
			log(LOG_DEBUG, "event slot:%d, monitor type :%d", (*it).first, type); 
			return mt;
		}
	}
	return NULL;
}
/*
SpreadMonitor* Pumping::get_SPmonitor(){
	SpreadMonitor* mt = static_cast<SpreadMonitor*>(get_monitor_bytype(SPREAD));
	assert(mt!= NULL);
	return mt;
}
*/


