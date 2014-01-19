#include <stdio.h>
#include <assert.h>
#include <poll.h>
#include <sys/time.h>
#include "SpreadMonitor.h"
#include "Pumping.h"
#include "MsgProto.h"
#include "Utils.h"

#define PRIVATENAME     "pgrepl"
#define REPLGROUP       "replgroup"
#define JOIN_GROUP_TIMEO    20000

#define SUBSCRIBE       1
#define UNSUBSCRIBE     0
#define PGREPL_MSG_TYPE    6

SpreadMonitor::SpreadMonitor(SQLloader* loader){
	mbox = -1;
	num_group=0;
	sqlloader = loader;
}

SpreadMonitor::~SpreadMonitor(){
	SP_quit(mbox, (char*)REPLGROUP);
}


int SpreadMonitor::init(Pumping* pump){
	Monitor::init(pump);
	int ret = SP_conect();
	return ret;
}

int SpreadMonitor::get_fd(){
	return mbox;
}

int SpreadMonitor::get_monitor_type(){
	return SPREAD;
}

int SpreadMonitor::IO_event_callback(int fd){
	assert(get_fd() == fd);
	log(LOG_DEBUG, "Spread event");	
	service srv_type = DROP_RECV;
	int endian = 0;
	char sender_pgrp[MAX_GROUP_NAME];
	
	int16 msg_type;
	SP_event_t sp_event;
	memset(inbuff, 0x0, MAX_MSGLEN);

	int size = SP_receive(fd, &srv_type, sender_pgrp, MAXNSPGROUP, \
									&num_group, groups, &msg_type, &endian, MAX_MSGLEN, (char*)inbuff);
	if ((size == ILLEGAL_SESSION) || (size == ILLEGAL_MESSAGE) || (size == CONNECTION_CLOSED)) {
		//sp_errno = SPUERR_SYSTEM;
		log(LOG_CRIT, "Spread is wrong state, trying to restart");
		SP_error(size);
		//SP_quit(fd,REPLGROUP);
		AppState::set_state(STATE_ADMINUP);
		return	 -1;
	}
	
	log(LOG_DEBUG, "Spread receive message size: %d,  srv_type: %d, sender: %s, msg_type: %d, num_group:%d",
			size,	srv_type,  sender_pgrp, msg_type, num_group);
	for(int i=0; i< num_group; i++){
		log(LOG_DEBUG, "num:%d, name: %s", i,	groups[i]);
	}
	if(!num_group){
		//*sp_event = SP_UNKNOWN_MESS;
		log(LOG_ERR, "unknown SP message");
		return SPREAD;
	}
	if ((size >= 0) && Is_regular_mess(srv_type)){
		sp_event = SP_DATA;
        if(msg_type == PGREPL_MSG_TYPE ){
            char *p1 = strrchr(sender_pgrp, '#');
            char *p2 = strrchr(self_group, '#');
            if(p1 == NULL || !strcmp(p1, p2)){   //broadcast app msg came from itself, ignore
				log(LOG_DEBUG, "this message coming frmo itself, ignored");            
                return SPREAD;
            }
            process_SP_mess(inbuff, size);
        }
	} 
	sp_event = SP_UNKNOWN_MESS;
	if ((size >= 0) && Is_reg_memb_mess(srv_type) ){
		if(Is_caused_join_mess(srv_type)){
			sp_event = SP_MEMBER_JOIN;
		}else if(Is_caused_leave_mess(srv_type)){
			sp_event = SP_MEMBER_LEAVE;
		}else if(Is_caused_disconnect_mess(srv_type) ){
			sp_event = SP_MEMBER_DISC;
		}else if(Is_caused_network_mess(srv_type) ){
			sp_event = SP_MEMBER_DISC;
		}else{
			log(LOG_CRIT, "this Is_reg_memb_mess event is not captured, please check :srv_type");
		}

	} else if ((size >= 0) && Is_membership_mess(srv_type) ){
		sp_event = SP_MEMBER_DISC;
	    log(LOG_CRIT, "this Is_membership_mess event is not captured, please check :srv_type");
		//return size;
	} 
	if(!strcmp(sender_pgrp, REPLGROUP)){		
		SP_members_change(sp_event) ;
	}
	return SPREAD;
}


int SpreadMonitor::SP_members_change(SP_event_t event){
    if (event == SP_MEMBER_JOIN){
        log(LOG_CRIT, "somebodys join the replication group");
    }else if(event == SP_MEMBER_DISC){
        log(LOG_DEBUG, "somebody leaves from  the replication group");
    }else{
        log(LOG_CRIT, "skiping SP invalid message");
    }
	return 0;
}

int SpreadMonitor::process_SP_mess(unsigned char* inbuff, int size){
	log(LOG_DEBUG, "spread recv:%s", inbuff);
	SP_Msg msg;
	int olen, msg_type;
	msg_type = msg.decode(inbuff, size, outbuff, &olen);
	if(msg_type == SQL_IMPORT){
		return sqlloader->load_SQL(outbuff, olen);	
	}
}

int SpreadMonitor::SP_conect(){

    char private_group[MAX_GROUP_NAME];
    int retry = 10;
    int ret;       
    // 1) connect spread  3 retries
    while( retry >= 0){
        ret = SP_connect("4803", PRIVATENAME, 0, SUBSCRIBE, &mbox, private_group);
        if(ret >=0 ){
            break;
        }else{
            SP_error(ret);
            log(LOG_CRIT, "Fail to establish connection to spread. Attempt: %d, secs:%d", 11- retry, (10-retry)*2);
            retry--;
            sleep(2);
        }
    }
    if(ret < 0){
        return APP_FAILURE;
    }
    
    // 2) join Cluster group        
    ret= SP_join(mbox, REPLGROUP);
    if(ret  <0){
        SP_error(ret);
        log(LOG_CRIT, "Can't join spread Cluster group");
        return APP_FAILURE;
    }
    strcpy(self_group, private_group);
   
    // 3) receive the member join message 
    char sender_pgrp[MAX_GROUP_NAME];
    int size;
    int16 msg_type;
    SP_event_t event;
    while(1){
        size = SP_recv_timeo(mbox, sender_pgrp, &msg_type,
            MAX_MSGLEN, (char*)inbuff, &event, &num_group, JOIN_GROUP_TIMEO);
        if(size <=0){
            SP_error(size);        
            log(LOG_CRIT,  "pgrepl can't received any response from spread when it join cluster group");    	
            return APP_FAILURE;    
        }else{
            if(!strcmp(sender_pgrp, REPLGROUP)   &&  event == SP_MEMBER_JOIN){ 
                if(num_group >= 1){ 
                    return num_group;
                }
            }
        }
    }        
    return APP_FAILURE;
}

int SpreadMonitor::SP_recv_timeo(mailbox mbox, char sender[MAX_GROUP_NAME],
        int16 *msg_type, int buf_size, char *inbuff, SP_event_t* sp_event,  int* num_group,  int timeout) 
{
    struct timeval tstart, tnow;
    struct pollfd pfd;
    
    int time_before_expr = timeout;

    gettimeofday(&tstart, NULL);

    pfd.fd = mbox;
    pfd.events = POLLIN;

    do {
        int res = poll(&pfd, 1, time_before_expr);
        if (res == 0) {
            //sp_errno = SPUERR_TIMED_OUT;
            return  -1;
        }
        /* socket error */
        if ((res < 0) && (errno != EINTR)) {
            //sp_errno = SPUERR_POLL_ERR;
            return  -2;
        }
        if (pfd.revents & POLLIN) {
            service srv_type = DROP_RECV;
            int endian = 0;
            int size, i;
            memset(inbuff, 0x0,  buf_size);
            size = SP_receive(
                mbox, &srv_type, sender, MAXNSPGROUP, num_group, groups,
                msg_type, &endian, buf_size, inbuff  );
            if ((size == ILLEGAL_SESSION) || (size == ILLEGAL_MESSAGE)  || (size == CONNECTION_CLOSED)) {
                //sp_errno = SPUERR_SYSTEM;
                SP_error(size);
                log(LOG_CRIT, "SP_receive error ");
                return   -3;
            }

            log(LOG_DEBUG,  "SP_recv_timeo receive message size: %d,  srv_type: %d, sender: %s, msg_type: %d, event:%d, num_group:%d",
                    size,   srv_type,  sender, *msg_type, *sp_event, *num_group);
            for(i=0; i< *num_group; i++){
                log(LOG_DEBUG,"group[%d]=%s", i,  groups[i]);
            }
            if(0 == *num_group){
                *sp_event = SP_UNKNOWN_MESS;
                return size;
            }
            if ((size >= 0) && Is_regular_mess(srv_type)){
                *sp_event = SP_DATA;
                return size;
            } else if ((size >= 0) && Is_reg_memb_mess(srv_type) ){
                if(Is_caused_join_mess(srv_type)){
                    *sp_event = SP_MEMBER_JOIN;
                }else if(Is_caused_leave_mess(srv_type)){
                    *sp_event = SP_MEMBER_LEAVE;
                }else if(Is_caused_disconnect_mess(srv_type) ){
                    *sp_event = SP_MEMBER_DISC;
                }else if(Is_caused_network_mess(srv_type) ){
                    *sp_event = SP_MEMBER_DISC;
                }else{
                    log(LOG_CRIT, "this Is_reg_memb_mess event is not captured, please check :srv_type");
                }
                return size;
            } else if ((size >= 0) && Is_membership_mess(srv_type) ){
                *sp_event = SP_MEMBER_DISC;
                log(LOG_CRIT, "this Is_membership_mess event is not captured, please check :srv_type");
                return size;
            } 

        }
        gettimeofday(&tnow, NULL);
        time_before_expr = timeout - ( 1000 * (tnow.tv_sec - tstart.tv_sec)
            + (int)(0.001 * ( tnow.tv_usec - tstart.tv_usec )));
        if (time_before_expr < 0) {
            return  -1;
        }
    } while(1);
}

void SpreadMonitor::SP_quit(mailbox box, char* group_name){
    int ret;    
    if(group_name != NULL){    
        int ret = SP_leave(box, group_name);
        if(ret < 0){
            log(LOG_INFO, "pgrepl fails to leave %s group", group_name);
        }
    }
    ret = SP_disconnect(box);
    if(ret < 0){
        log(LOG_INFO, "pgrepl fails to disconnect  spread, spread died?");
    }        
}

int SpreadMonitor::SP_mcast(unsigned char* inbuff, int size){
	log(LOG_DEBUG, "call SP_mcast"); 
    int ret = SP_multicast(mbox, FIFO_MESS, REPLGROUP, PGREPL_MSG_TYPE, size, (const char*)inbuff);
    if (ret < size) {
		log(LOG_DEBUG, "SP_multicast failed:%d, return%d", size, ret);		
        SP_error(ret);
        return -1;
    }
    log(LOG_DEBUG, "SP_multicast input size:%d, return%d", size, ret);   
    return ret;	
}

int SpreadMonitor::get_SP_group(unsigned char* group_list){
	sprintf((char*)group_list, "num:%d\n", num_group);
	for(int i=0;i< num_group; ++i){
		strcat((char*)group_list,groups[i]);
		strcat((char*)group_list,"\n");
	}
	return num_group;
}




