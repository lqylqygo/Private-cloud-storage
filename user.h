#ifndef __USER_H__
#define __USER_H__ 
#include <my_header.h>
typedef enum{
    STATUS_LOGOFF=0,
    STATUS_LOGIN
}LoginStatus;

typedef struct {
    int sockfd;//套接字文件描述符
    LoginStatus status;//登录状态
    char username[20];//用户名
    char encrypted[100];//etc/shadow中获取的加密密文
    char pwd[128];//用户当前路径
} user_info_t;

void getsetting(char *setting,char *passwd);
void loginCheck1(user_info_t *user);
void loginCheck2(user_info_t *user,const char *encrypted);

#endif