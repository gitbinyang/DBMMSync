#include "ClntSockMonitor.h"
#include "Pumping.h"
#include "SpreadMonitor.h"

#define SP_CONF "/etc/spread.conf"
ClntSockMonitor::ClntSockMonitor(int clnt_fd){
	ClntSock_fd = clnt_fd;
	set_IO_unblocking(ClntSock_fd);
}

ClntSockMonitor::~ClntSockMonitor(){
	close(ClntSock_fd);
}

int ClntSockMonitor::init(Pumping* pump){
	Monitor::init(pump);
	return 1;
}

int ClntSockMonitor::get_fd(){
	return ClntSock_fd;
}

int ClntSockMonitor::get_monitor_type(){
	return CLNTSOCK;
}


int ClntSockMonitor::IO_event_callback(int fd){
	assert(get_fd() == fd);	
	Monitor::IO_event_callback(fd);
	int nbytes =  read(fd, inbuff, MAX_MSGLEN);
	int ret;
	if(nbytes<=0){
		log(LOG_DEBUG, "client socket fd:%d is closed, error:%s", fd, strerror(errno));
		close(fd);
		return -1;
		
	}else{
		inbuff[nbytes]='\0';
		memset(outbuff, 0x0, MAX_MSGLEN);
		char* listgroup[] = {"LIST GROUP", "NEW CONF:", "EXITING"};
		//0: "LIST GROUP"
		if(!strncasecmp((const char*)inbuff, listgroup[0], strlen(listgroup[0]))){
			SpreadMonitor* mt = dynamic_cast<SpreadMonitor*>(O_pump->get_monitor_bytype(SPREAD));
			assert(mt != NULL);
			
			int num = mt->get_SP_group(outbuff);
			if(!num){
				strcat((char*)outbuff, "0\n");
			}
			ret = write(fd, (const char*)outbuff, strlen((const char*)outbuff));	
		//1:"NEW CONF:"	
		}else if(!strncasecmp((const char*)inbuff, listgroup[1], strlen(listgroup[1]))){
			strcat((char*)outbuff, "OK: receive spread conf\n");
			ret = write(fd, (const char*)outbuff, strlen((const char*)outbuff));	
			log(LOG_DEBUG, "new conf:%s",inbuff);
			int n = strlen(listgroup[1]);
			ret = Utils::getInstance().reload_SP_conf(SP_CONF, (const char*)(inbuff + n), nbytes - n);
			
		//2:"EXITING"
		}else if(!strncasecmp((const char*)inbuff, listgroup[2], strlen(listgroup[2]))){
			strcat((char*)outbuff, "OK: exiting\n");
			ret = write(fd, (const char*)outbuff, strlen((const char*)outbuff));	
			log(LOG_ALERT, "received a exiting command");
			AppState::set_state(STATE_CLOSED);
		//invalid	
		}else{
			strcat((char*)outbuff, "ERR: invalid command\n");
			ret = write(fd, (const char*)outbuff, strlen((const char*)outbuff));
		}
		
	}
	return CLNTSOCK;
}




