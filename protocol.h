#ifndef MY__PROTOCOL__H
#define MY__PROTOCOL__H

#include<my_header.h>

typedef enum{
    CMD_PWD=1,
    CMD_CD,
    CMD_LS,
    CMD_PUTS,
    CMD_GETS,
    CMD_REMOVE,//删除
    CMD_INVALID
}CommandType;

//TLV结构体
typedef struct{
    int type;//命令类型
    int length;//数据长度
}TLVHeader;
#endif
