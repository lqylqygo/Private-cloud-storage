// protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <my_header.h>

typedef enum {
    CMD_PWD = 1, 
    CMD_CD, 
    CMD_LS, 
    CMD_REMOVE,
    
    CMD_PUTS,       
    CMD_PUTS_DATA,  // TLV的类型
    CMD_PUTS_END,   // TLV的类型
    
    CMD_GETS,       
    CMD_GETS_DATA,  
    CMD_GETS_END,   
    
    CMD_INVALID
} CommandType;

typedef struct {
    int type;
    int length;
} TLVHeader;

#endif