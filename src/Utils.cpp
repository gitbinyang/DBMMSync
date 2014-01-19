#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include "Utils.h"
#include "Config.h"



#define SYSLOG_FACILITY 40 << 3

int Utils::init(){
	init_log();
	return read_eth0_ip();
}

Utils& Utils::getInstance(){
	static Utils instance;
	return instance;
}

int Utils::set_user_mode(){
    struct passwd *pwd=NULL;
    uid_t uid;
    gid_t gid;

    pwd = getpwnam ("postgres");
    if (pwd == NULL)  {
        log(LOG_CRIT,"User postgres hasn't been created\n");
        return APP_FAILURE;
    }

    if ((setgid (pwd->pw_gid) == -1) || ((setuid (pwd->pw_uid)) == -1)) {
        log(LOG_CRIT, "privilege change error:%s\n", strerror(errno));           
        return APP_FAILURE;       
    }
    uid = getuid (); gid = getegid ();
    

    if (uid && uid != pwd->pw_uid){
        log(LOG_CRIT,"Uid set incorrectly");
        return APP_FAILURE;
    }
    if (gid && gid != pwd->pw_gid) {
        log(LOG_CRIT,"Gid set incorrectly");
        return APP_FAILURE;
    }
    log(LOG_CRIT,"pgrepl run as postgres user MODE");
    return APP_SUCCESS;
}    


void Utils::init_log() {

  openlog("pgrepl", LOG_PID, SYSLOG_FACILITY);

  strcpy(log_types[LOG_EMERG],"EMERG");
  strcpy(log_types[LOG_ALERT],"ALERT");
  strcpy(log_types[LOG_CRIT],"CRIT");
  strcpy(log_types[LOG_ERR],"ERR");
  strcpy(log_types[LOG_WARNING],"WARN");
  strcpy(log_types[LOG_NOTICE],"NOTICE");
  strcpy(log_types[LOG_INFO],"INFO");
  strcpy(log_types[LOG_DEBUG],"DEBUG");

}

void write_private(const char* line, int len){
	FILE *f=NULL;		
	f = fopen("/tmp/pgrepl.log", "a+");
	if(!f){
		printf("unable open log, error:%s", strerror(errno));
		exit(-1);
	}
	fwrite(line, len, 1, f);
	fclose(f);
}

void log(int level, const char *fmt,...) {
    if(Config::getInstance().get_log_level()>= level){  
        char line[10240];
        va_list argp;
        int c=0;

        sprintf(line,"[%s]",Utils::getInstance().log_types[level]);

        c += snprintf(line + c, sizeof(line) - c, "[%s] ",Utils::getInstance().log_types[level]);
        va_start(argp, fmt);
        c += vsnprintf(line + c, sizeof(line) - c, fmt, argp);
        va_end(argp);
        c += snprintf(line + c, sizeof(line) - c, "\n");
        //FIXME later
        //syslog(level,line);
        write_private(line, strlen(line));
    }
}

int Utils::read_eth0_ip(){
    struct ifreq   buffer[32];
    struct ifconf intfc;
    struct ifreq  *pIntfc;
    int	i, fd, num_intfc;
    
    intfc.ifc_len = sizeof(buffer);
    intfc.ifc_buf = (char*) buffer;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        log(LOG_CRIT, "socket() failed");
        return -1;
    }

    if (ioctl(fd, SIOCGIFCONF, &intfc) < 0){
        close(fd);        
        log(LOG_CRIT,"ioctl SIOCGIFCONF failed");
        return -1;
    }
    int ret = APP_FAILURE;	
    pIntfc    = intfc.ifc_req;
    num_intfc = intfc.ifc_len / sizeof(struct ifreq);
    struct ifreq *item = NULL;
    
    for (i = 0; i < num_intfc; i++){
	    item = &(pIntfc[i]);
        if(!strcasecmp(item->ifr_name, "eth0")){
            strcpy(str_ip, inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));
			int_ip =  inet_addr((const char*)str_ip); 
            ret =  APP_SUCCESS;
            break;
        }
    }
    close(fd);    
	
    return ret;
}

const char* Utils::get_str_ip(){
	return str_ip;
}

uint32_t Utils::get_int_ip(){
	return int_ip;
}

void Utils::daemonize(){
    int i;
    int status = -1;
    struct rlimit resourceLimit = { 0 };

    status = fork();
    switch (status)
    {
    case -1:
        log(LOG_CRIT, "Fork to create daemon failed with error");
        exit(APP_FAILURE);
    case 0:
        break;
    default:
        exit(APP_SUCCESS);
    }

    resourceLimit.rlim_max = 0;
    status = getrlimit(RLIMIT_NOFILE, &resourceLimit);
    if (-1 == status)   {
        log(LOG_CRIT, "getrlimit returns error");
        exit(APP_FAILURE);
    }
    if (0 == resourceLimit.rlim_max)  {
        log(LOG_CRIT, "Max number of open file descriptors is 0!!\n");
        exit(APP_FAILURE);
    }
    
    for (i = 0; i < resourceLimit.rlim_max; i++)
      (void) close(i);

    status = setsid();
    if (-1 == status)  {
        log(LOG_CRIT, "Setsid failed with error");
        exit(APP_FAILURE);
      }
    status = fork();
    switch (status)  {
    case -1:
        log(LOG_CRIT, "second attempt at fork failed with error");
        exit(1);
    case 0: 
		log(LOG_DEBUG, "running a daemon process");
        break;
    default: 
        exit(APP_SUCCESS);
    }
}

int Utils::reload_SP_conf(const char* fname, const char* line, int len){
	if( write_new_file(fname,line,len) > 0){
		log(LOG_ALERT, "spread config file is updated, it restarting");
		system("/etc/init.d/spread restart");				
	}
	return -1;
}

int Utils::write_new_file(const char* fname, const char* line, int len){
	FILE *f=NULL;		
	f = fopen(fname, "w+");
	if(!f){
		log(LOG_CRIT, "unable open file %s, error:%s", fname, strerror(errno));
		return -1;
	}
	fwrite(line, len, 1, f);
	fclose(f);
	return 1;
}



