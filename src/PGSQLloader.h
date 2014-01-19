/*
 * PGSQLloader.h
 *
 *  Created on: Jan 19, 2014
 *      Author: yangb
 */

#ifndef PGSQLLOADER_H_
#define PGSQLLOADER_H_
#include <libpq-fe.h>
#include "SQLloader.h"

class PGSQLloader: public SQLloader {
public:
	PGSQLloader();
	virtual ~PGSQLloader();
    virtual int filter_DB_SQL(unsigned char* inbuff, int len);
    virtual int load_SQL(unsigned char* inbuff, int len);
    virtual int init(Config& config);
    virtual void debug();
protected:
    int parse_delimitor_string(VECTOR& , const char* s, char delimitor);
    int search_prefix(const char* s);
    int search_leading_key(VECTOR& v, const char* s);
private:
    int PG_connect();
    void PG_close() ;
    PGconn * db_handle;
};

#endif /* PGSQLLOADER_H_ */
