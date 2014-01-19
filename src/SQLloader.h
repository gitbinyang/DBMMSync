#ifndef SQLLOAD_H
#define SQLLOAD_H

#include <vector>
#include "Config.h"

typedef std::vector<char*> VECTOR;

class SQLloader{
public:
    SQLloader();
    virtual ~SQLloader();
    virtual int filter_DB_SQL(unsigned char* inbuff, int len)=0;
    virtual int load_SQL(unsigned char* inbuff, int len)=0;
    virtual int init(Config& config)=0;
    virtual void debug()=0;
protected:
    int parse_delimitor_string(VECTOR& , const char* s, char delimitor);
    int search_prefix(const char* s);
    int search_leading_key(VECTOR& v, const char* s);
    VECTOR  v_sql;
    VECTOR  v_prefix;
    char SQL_url[256];        
};

#endif

