#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <syslog.h>

#include "Utils.h"
#include "Config.h"


#define PGREPL_CONF  "/etc/pgrepl.conf"
const int LONGS_LEN=1024;

Config::Config(){
	matched_logpath[0]='\0';
}


Config& Config::getInstance(){
	static Config instance;
	return instance;
}

int Config::init(){
	return read_node_cnf();
}

int Config::read_node_cnf(){
    FILE *cfgfptr=NULL;
    char str[LONGS_LEN];
    if (!(cfgfptr = fopen(PGREPL_CONF, "r"))){
        log(LOG_CRIT,"%s file open error: %s, return default value\n", PGREPL_CONF, strerror(errno));
        return  APP_FAILURE;
    }
    char *p = NULL;
    const char * pentry;
	int len=0;
    while((fgets(str, LONGS_LEN, cfgfptr)) != (char *)0 ) {
		len = strlen(str);
		if(str[len-1] == '\n'){ //chomp off 
			str[len-1] = '\0';				
		}
        p=strstr(str,"LogLevel=");
        if(p){
            pentry =  p + strlen("LogLevel=" );
            log_level = atoi(pentry);
            continue;
        }      
        p=strstr(str,"LogName=");
        if(p){
            pentry =  p + strlen("LogName=" );
            strcpy(matched_logname, pentry);
            continue;
        } 		
        p=strstr(str,"LogPath=");
        if(p){
            pentry =  p + strlen("LogPath=" );
            strcpy(matched_logpath, pentry);
            continue;
        } 		

        p=strstr(str,"Database=");
        if(p){
            pentry =  p + strlen("Database=" );
            strcpy(matched_DBname, pentry);
            continue;
        } 		

        p=strstr(str,"PGUser=");
        if(p){
            pentry =  p + strlen("PGUser=" );
            strcpy(matched_PGUser, pentry);
            continue;
        } 		
		
        p=strstr(str,"SQL=");
        if(p){
            pentry =  p + strlen("SQL=" );
            strcpy(matched_SQL, pentry);
            continue;
        }      
        p=strstr(str,"PREFIX=");
        if(p){
            pentry =  p + strlen("PREFIX=" );
            strcpy(matched_prefix, pentry);
            continue;
        }      
        p=strstr(str,"SQLNum=");
        if(p){
            pentry =  p + strlen("SQLNum=" );
            SQL_num = atoi(pentry);
            continue;
        }      
		
    }  
    fclose(cfgfptr); 
	
    return APP_SUCCESS;    
}


const char* Config::get_matched_SQL(){
	return matched_SQL;
}

const char* Config::get_matched_prefix(){
	return matched_prefix;
}


/*
inline int Config::get_log_level(){
	return log_level;
}*/


