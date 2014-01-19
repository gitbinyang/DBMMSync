#include "SQLloader.h"

SQLloader::SQLloader(){
}

SQLloader::~SQLloader(){

	while (!v_sql.empty()) {
	    delete v_sql.back();
	    v_sql.pop_back();
	}

	while (!v_prefix.empty()) {
		delete v_prefix.back();
		v_prefix.pop_back();
	}

}

