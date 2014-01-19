/*
 * PGSQLloader.cpp
 *
 *  Created on: Jan 19, 2014
 *      Author: yangb
 */
#include <string.h>
#include "PGSQLloader.h"
#include "Utils.h"

#define DELIMITOR '#'

PGSQLloader::PGSQLloader() {
	db_handle=NULL;
}

PGSQLloader::~PGSQLloader() {
	PG_close();
}

int PGSQLloader::init(Config& config){
	parse_delimitor_string(v_sql, config.get_matched_SQL(), '#');
	parse_delimitor_string(v_prefix,config.get_matched_prefix(), '#');
	//"host='localhost' dbname='mytest' user='spread' password='spread'";
	sprintf(SQL_url, "host=\'localhost\' dbname=\'%s\' user=\'spread\' password=\'spread\'", config.get_matched_DBname());
	log(LOG_DEBUG, "SQL_url:%s", SQL_url);
	return PG_connect();
}

//open PG connect
int PGSQLloader::PG_connect() {

	db_handle=PQconnectdb(SQL_url);
	if(PQstatus(db_handle) != CONNECTION_OK){
		log(LOG_DEBUG, "Connection to database failed: %s",PQerrorMessage(db_handle));
		PG_close();
		return APP_FAILURE;
	}
	log(LOG_DEBUG,"PG connection OK, user:spread");
	return APP_SUCCESS;
}
void PGSQLloader::PG_close() {
	if(PQstatus(db_handle) == CONNECTION_OK){
		PQfinish(db_handle);
	}
}

//sql is totally static, insert it to DB
int PGSQLloader::load_SQL(unsigned char*sql, int len){
    PGresult * pgr;

	if(sql[len-1]==';'){  //remove ; from tail
		sql[len-1] = '\0';
		len--;
	}
    pgr=PQexec(db_handle, (const char*)sql);

    if (PQresultStatus(pgr) != PGRES_COMMAND_OK)  {
		log(LOG_ERR, "SQL failed:%s", sql);
		log(LOG_ERR, "error: %s", PQerrorMessage(db_handle));
		PQclear(pgr);
        return APP_FAILURE;
    }
	return APP_SUCCESS;
}


//input: parse a string separated by delimiter, and extract each field and push to pv vector
int PGSQLloader::parse_delimitor_string(VECTOR& pv, const char* s, char delimitor){
	const char* ptr1 = s;
	const char* ptr2 = ptr1;
	int len;
	char * pt;
	while(ptr2 != NULL){
		ptr2 = strchr(ptr1,delimitor);
		if(ptr2){
			len  = ptr2 - ptr1;
			pt = new char[ len + 1];
			memcpy(pt, ptr1, len);
			pt[len]='\0';

			pv.push_back(pt);

		}else{
			len = strlen(ptr1);
			if(ptr1[len -1 ] == '\n')
				len--;
			pt = new char[ len + 1];
			memcpy(pt, ptr1, len);
			pt[len]='\0';

			pv.push_back(pt);
			break;
		}
		ptr1 = ptr2 + 1;
	}


}


//input:
//postgresqlLOG: statement: insert xxxx
//useless log: xxxxx
//postgresqlLOG: statement: update xxxx

//return: fill inbuff with new string insert xxxx; update xxxx;
//return new size


int PGSQLloader::filter_DB_SQL(unsigned char* inbuff, int buff_len){
	char* ptr1 = (char*)inbuff;
	char* ptr2 = ptr1;
	//int len;
	int size=0; // return size
	//char * pt;
	//VECTOR v_tmp;
	while(ptr2 != NULL){
		ptr2 = strchr(ptr1, '\n');
		if(ptr2){
			*ptr2 = '\0';
			int ret = search_leading_key(v_prefix, ptr1);  //search prefix

			if(ret > 0){                                  //found search prifx
				ptr1 += ret;
				while(*ptr1 == ' ') ptr1++;
				if(search_leading_key(v_sql, ptr1)>0){     //found search sql key words
					int len  = ptr2 - ptr1;
					//inbuff[size]='\0';
					//pt = new char[ len + 1];
					memcpy(inbuff+size, ptr1, len);
					size +=len;
					if(inbuff[size-1] !=';'){
						inbuff[size] = ';';
						size +=1;
					}

				}
			}

			ptr1 = ptr2 + 1;
		}
	}
	inbuff[size]='\0';
	log(LOG_DEBUG, "debug filter_DB_SQL output:###%s####", inbuff);
	//2)
	return size;

}

//check if s starts with one of  prefixes, and
//return the prefix length
inline int PGSQLloader::search_leading_key(VECTOR& v, const char* s){
	for(uint16_t i=0; i < v.size() ; ++i){
		if(!strncasecmp(s, v[i], strlen(v[i]) )) {
			log(LOG_DEBUG, "found prefix :%s", v[i]);
			return strlen( v[i]);
		}

	}
	return 0;
}

void PGSQLloader::debug(){

	for(uint16_t i=0; i < v_sql.size() ; ++i){
		log(LOG_DEBUG, "sql %d:%s", i, v_sql[i]);
	}
	for(uint16_t i=0; i < v_prefix.size() ; ++i){
		log(LOG_DEBUG, "prefix %d:%s", i, v_prefix[i]);
	}

}

