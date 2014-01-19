#include "FileMonitor.h"
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <assert.h>

#include "Utils.h"
#include "SpreadMonitor.h"
#include "Pumping.h"
#include "Config.h"
#include "MsgProto.h"


#define _60K          1024*60
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( MAX_SOCK * ( EVENT_SIZE + 16 ) )


//class Pumping;


FileMonitor::FileMonitor(SQLloader* loader){
    memset(watch_file, 0x0, 256);
	memset(error_message, 0x0, 1024);
	inotify_fd   = -1;
	inotify_wd   = -1;
    watch_filefd = -1;
    cur_offset   = -1;	
	sqlloader    = loader;
}


FileMonitor::~FileMonitor(){
	( void ) inotify_rm_watch( inotify_fd, inotify_wd );
	( void ) close( inotify_fd );

	close(watch_filefd);
	log(LOG_DEBUG, "FileMonitor destructor");	  
}

int FileMonitor::init(Pumping* pump){
	Monitor::init(pump);	
	if( (inotify_fd = inotify_init()) > 0){ 	
		inotify_wd = inotify_add_watch( inotify_fd, Config::getInstance().get_matched_logpath(), 
                         IN_MODIFY | IN_CREATE | IN_DELETE );		
		set_IO_unblocking(inotify_fd);
		if(guess_watch_file() > 0){
			cur_offset = seek_file_offset(0, SEEK_END);
			
			log(LOG_CRIT,"watch file %s", watch_file);
			return APP_SUCCESS;
		}
	}
	log(LOG_CRIT,"file monitor failed");
	return APP_FAILURE;
	
}

int FileMonitor::get_fd(){
	return inotify_fd;
}

int FileMonitor::get_monitor_type(){
	return PGFILE;
}


int FileMonitor::IO_event_callback(int fd){
	Monitor::IO_event_callback(fd);
	assert(get_fd() == fd);
	log(LOG_DEBUG, "PG log file event");	
	int num = 0;
	char buffer[BUF_LEN];
	int length = read(inotify_fd, buffer, BUF_LEN);
	if(length < 0){
		log(LOG_ERR, "read inotify error");
		return -1;
	}
	char tmp[256];
	while ( num < length ) {
	  struct inotify_event *event = ( struct inotify_event * ) &buffer[ num ];
	  if ( event->len ) {
		sprintf(tmp, "%s%s", Config::getInstance().get_matched_logpath(), event->name);
		if(!strcmp(tmp, watch_file)){

			if ( event->mask & IN_CREATE ) {
			  if ( event->mask & IN_ISDIR ) {
				log(LOG_DEBUG, "The directory %s was created.\n", event->name );		
			  }
			  else {
				log(LOG_DEBUG,"The file %s was created.\n", event->name );
			  }
			}else if ( event->mask & IN_DELETE ) {
			  if ( event->mask & IN_ISDIR ) {
				log(LOG_DEBUG,"The directory %s was deleted.\n", event->name );		
			  }
			  else {
				log(LOG_DEBUG,"The file %s was deleted.\n", event->name );
			  }
			}else if ( event->mask & IN_MODIFY ) {
			  if ( event->mask & IN_ISDIR ) {
				log(LOG_DEBUG,"The directory %s was modified.\n", event->name );
			  }
			  else {
				log(LOG_DEBUG,"The file %s was modified.\n", event->name );
				tail_lines();
			  }
			}
		  }
	  }
	  num += EVENT_SIZE + event->len;
	}

	
	return PGFILE;
}

//return current offset
off_t FileMonitor::seek_file_offset(off_t offset, int whence){
	
	if(watch_filefd <= 0){ 
		watch_filefd = open(watch_file, O_RDONLY);
		if(watch_filefd < 0){
			log(LOG_ERR, "tail lines error:%s", strerror(errno));
			return -1;
		}
				
    }
	//int last_offet = cur_offset;
	off_t new_offset = lseek(watch_filefd, offset, whence);
	log(LOG_DEBUG, "current offset %d", new_offset);	
	return new_offset;
}


//return the file being monitored if success
//return NULL if any failure
int FileMonitor::guess_watch_file(){
    struct stat     statbuf;
    struct dirent   *dirp;
    DIR             *dp;

    if ((dp = opendir(Config::getInstance().get_matched_logpath())) == NULL){
        snprintf(error_message,1024, "opendir failed:%s", strerror(errno));
		log(LOG_CRIT, "opendir failed:%s", strerror(errno));
        return -1;
    }
    char each_file[256];
    
    time_t mtime=0;
    while ((dirp = readdir(dp)) != NULL) {
         if (strcmp(dirp->d_name, ".") == 0 ||
             strcmp(dirp->d_name, "..") == 0 )
                 continue; 
         sprintf(each_file, "%s%s", Config::getInstance().get_matched_logpath(), dirp->d_name);
         if(stat(each_file, &statbuf) < 0){
             log(LOG_DEBUG, "state file:%s fail", each_file);
             continue;
         }
         if (S_ISDIR(statbuf.st_mode) != 0){
             log(LOG_DEBUG, "%s is not file, ", each_file);
             continue;
         }
		 //log(LOG_DEBUG, "guess_watch_file debug:%s", Config::getInstance().get_matched_logname());
         if(strstr(dirp->d_name, "postgresql") == NULL || strstr(dirp->d_name,".log")==NULL){
             log(LOG_DEBUG, "%s is not postgres log file", dirp->d_name);
             continue;
         }  
         if(statbuf.st_mtime > mtime){
             mtime = statbuf.st_mtime;
             strcpy(watch_file, each_file);             
         }
	}
    closedir(dp);
    if(mtime) //there is no valid file
		return 1;
	else
		return 0;
		
}


const char* FileMonitor::get_watch_file(){
	return watch_file;
}

int FileMonitor::tail_lines(){
	off_t tail_offset = seek_file_offset( 0, SEEK_END);
	if(tail_offset < 0){
		log(LOG_ERR, "tail lines lseek failed %s", strerror(errno));
		return -1;
	}
	int left_bytes = tail_offset - cur_offset;       //total new bytes
	int nbytes;

	seek_file_offset(-left_bytes, SEEK_END);
	while(left_bytes > _60K){                         //maxium 60K
	
		nbytes = read(watch_filefd, inbuff, _60K);
		log(LOG_CRIT, "chunk read:%d", nbytes);
		if(nbytes < 0){
			log(LOG_CRIT, "tail lines error:%s", strerror(errno));
			return -1;
		}		
		assert(nbytes == _60K);
		inbuff[nbytes]='\0';
		char* delimitor = strrchr((char*)inbuff, '\n');
		assert(delimitor != NULL);

		int valid_len = delimitor - (char*)inbuff;  //chomp off the last half SQL
		inbuff[valid_len]='\0';
		cur_offset += valid_len;
		seek_file_offset(-(_60K - valid_len), SEEK_CUR); //rewind file pointer
		
		mcast_max_tail(inbuff, valid_len);
		
		left_bytes -=valid_len;
	}


	nbytes = read(watch_filefd, inbuff, left_bytes);
	log(LOG_CRIT, "read:%d", nbytes);
	if(nbytes < 0){
		log(LOG_CRIT, "tail lines error:%s", strerror(errno));
		return -1;
	}		
	assert(nbytes ==left_bytes);
	inbuff[nbytes]='\0';
	cur_offset += nbytes;
	return mcast_max_tail(inbuff, nbytes);

}


int FileMonitor::mcast_max_tail(unsigned char*inbuff, int nbytes){

	int ret = sqlloader->filter_DB_SQL(inbuff, nbytes);
	if(ret > 1){
		SP_Msg msg;
		int olen, ret;
		ret = msg.encode(SQL_IMPORT, inbuff, nbytes, outbuff, &olen);
		
		SpreadMonitor* mt = dynamic_cast<SpreadMonitor*>(O_pump->get_monitor_bytype(SPREAD));
		assert(mt != NULL);
		log(LOG_DEBUG, "SpreadMonitor addr:%p", mt);
		return mt->SP_mcast(outbuff, olen);
		
	}else{
		return ret;
	}
}


