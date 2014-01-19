#ifndef CONFIG_H
#define CONFIG_H
#include <string>

class Config{
public:
    static Config& getInstance();
    int init();
    int read_node_cnf();
    int get_log_level(){ return log_level;}
    const char* get_matched_logname(){ return matched_logname;}
    const char* get_matched_logpath(){ return matched_logpath;}
    const char* get_matched_DBname(){ return matched_DBname;}
    const char* get_matched_SQL(); 
    const char* get_matched_prefix();
    
private:
    Config();
    char matched_SQL[1024];
    char matched_prefix[1024];
    int  log_level;
    char matched_logname[32];
    char matched_logpath[256];
    char matched_DBname[32];
    char matched_PGUser[32];
    int  SQL_num;   
    
};

#endif
