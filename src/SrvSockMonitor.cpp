
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SrvSockMonitor.h"
#include "ClntSockMonitor.h"
#include "Pumping.h"


#include "Utils.h"


#define PORT 5885
SrvSockMonitor::SrvSockMonitor(){
	SrvSock_fd = -1;
}
SrvSockMonitor::~SrvSockMonitor(){
	close(SrvSock_fd);	
}

int SrvSockMonitor::init(Pumping* pump){
	Monitor::init(pump);
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = PF_INET;
	
	const char* ip = Utils::getInstance().get_str_ip();
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port = htons(PORT);

	if ((SrvSock_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		log(LOG_CRIT, "%s:%i failed to create socket: %s\n",ip,PORT,strerror(errno));
		return -1;
	}
	int on=1;
	if (setsockopt(SrvSock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		log(LOG_CRIT, "%s:%i setsockopt RESUADDER failed: %s\n",ip,PORT,strerror(errno));
		return -1;
	}

	if (set_IO_unblocking(SrvSock_fd)< 0) {
		log(LOG_CRIT,"%s:%i set_IO_unblocking failed: %s\n",ip,PORT,strerror(errno));
		return -1;
	}
	
	if (bind(SrvSock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		log(LOG_CRIT,"%s:%i failed to bind to port: %s\n",ip ,PORT,strerror(errno));
	  	return -1;
	}
	
	if (listen(SrvSock_fd, 5) == -1) {
		log(LOG_CRIT, "%s:%i listen failed: %s\n",ip,PORT,strerror(errno));
		return -1;
	}
	log(LOG_DEBUG, "admin server socket is listening ip:%s, port:%d, sockfd:%d", ip, PORT,SrvSock_fd);
	return APP_SUCCESS;
	
}
int SrvSockMonitor::get_fd(){
	return SrvSock_fd;
}
int SrvSockMonitor::get_monitor_type(){
	return SRVSOCK;	
}


int SrvSockMonitor::IO_event_callback(int fd){
	assert(get_fd() == fd);
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
	
	int clnt_sock = accept(SrvSock_fd, (struct sockaddr *)&cliaddr, &addrlen);	
	if(clnt_sock <0){
		log(LOG_CRIT, "admin socket accept failed: %s\n",strerror(errno));	
		return -1;
	}
	Monitor* mt = new ClntSockMonitor(clnt_sock);
	mt->init(O_pump);
	O_pump->add_monitor(mt);
	return SRVSOCK;
}

