#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H
#include <sys/types.h>
#include <unistd.h>
#include "Monitor.h"
#include "SQLloader.h"



class FileMonitor :public Monitor{
public:
    FileMonitor(SQLloader* loader);
    virtual ~FileMonitor();
    virtual int get_fd();    
    virtual int get_monitor_type();
    
    virtual int IO_event_callback(int fd);
    virtual int init(Pumping* pump);
    const char* get_watch_file();
    
private:
    int tail_lines();
    int guess_watch_file();
    off_t seek_file_offset(off_t offset, int whence);
    int mcast_max_tail(unsigned char*inbuff, int nbytes);

    char watch_file[256];
    char error_message[1024];
    int watch_filefd;
    int inotify_fd;
    int inotify_wd;
    off_t cur_offset;
    SQLloader* sqlloader;
        
};
#endif

