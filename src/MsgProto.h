#ifndef MSGPROTO_H
#define MSGPROTO_H

typedef enum{
    QURY_SP_GOURP=1, //admin socket message
    SQL_IMPORT=11,     //spread message
                
}ALL_MSG;


typedef struct msg_header{
    int msg_len;    
    int msg_type;    
}msg_header;

typedef struct {
    msg_header header;
    char buff[1024 * 10]; 
}Msg_QURY_SP_GOURP;


typedef struct {
    msg_header header;
    char buff[1024 * 60]; 
}Msg_SQL_IMPORT;



//parse all messages
class MsgProto{
public:

    virtual int encode(int msg_type, unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen )=0;
    //return message type
    virtual int decode(unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen )=0;
protected:
    msg_header* get_header(unsigned char*inbuff);
};


class SP_Msg : public MsgProto{
public:    
    virtual int encode(int msg_type, unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen );
    //return message type
    virtual int decode(unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen );

};

class Admin_Msg : public MsgProto{
public:    
    virtual int encode(int msg_type, unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen );
    //return message type
    virtual int decode(unsigned char* inbuff, int ilen, unsigned char* outbuff, int*olen );
    
};




#endif

