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
    
    CMD_INVALID,
     TASK_LOGIN_SECTION1 = 100,          // 第一次验证，客户端发送用户名
    TASK_LOGIN_SECTION1_RESP_OK,        // 服务器返回 salt 值
    TASK_LOGIN_SECTION1_RESP_ERROR,     // 用户名不存在
    TASK_LOGIN_SECTION2,                // 第二次验证，客户端发送加密后的密文
    TASK_LOGIN_SECTION2_RESP_OK,        // 登录成功
    TASK_LOGIN_SECTION2_RESP_ERROR      // 密码错误
} CommandType;

typedef struct {
    int type;
    int length;
} TLVHeader;

int sendn(int sockfd, const void *buf, size_t len);
#endif
