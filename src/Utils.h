#ifndef UTILS_H
#define UTILS_H
#include <unistd.h>

#include <stdint.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>

#define APP_FAILURE    - 1 
#define APP_SUCCESS    0
void log(int level, const char *fmt,...);

class Utils{
public:
    static Utils& getInstance();
    int init();
    int set_user_mode();
    const char* get_str_ip();
    uint32_t get_int_ip();
    void daemonize();
    int reload_SP_conf(const char* fname, const char* line, int len);
    char log_types[8][10];
private:    
    int write_new_file(const char* fname, const char* line, int len);
    
    Utils(){}
    void init_log();
    int read_eth0_ip();
    char str_ip[16];
    
    uint32_t    int_ip;
};
#endif



