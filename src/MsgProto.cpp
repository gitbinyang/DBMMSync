#include <assert.h>
#include <string.h>
#include "MsgProto.h"

msg_header* MsgProto::get_header(unsigned char*inbuff){
	msg_header* header = (msg_header*)inbuff;
	return header;
}

//return encode succes or not
int SP_Msg::encode(int msg_type, unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen ){
	int size;
	if(msg_type == SQL_IMPORT){
		size = sizeof(Msg_SQL_IMPORT);
		//assert(size == ilen);		
		memset(outbuff, 0x0, size);
		
		Msg_SQL_IMPORT* msg =(Msg_SQL_IMPORT*)outbuff;
		msg->header.msg_len = size;
		msg->header.msg_type= SQL_IMPORT;
		memcpy(msg->buff, inbuff, ilen);
	}	*olen = size;
	return 1;
}

//return message type
int SP_Msg::decode( unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen ){

	msg_header* header = get_header(inbuff);
	
	if(header->msg_type == SQL_IMPORT){
		
		Msg_SQL_IMPORT* msg = (Msg_SQL_IMPORT*)inbuff;
		memcpy(outbuff, msg->buff, header->msg_len);
		*olen = header->msg_len;	
	}
	return header->msg_type;

}











