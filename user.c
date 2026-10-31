#include "user.h"
#include "protocol.h"
void getsetting(char *setting, char *passwd) {
    int i, count = 0;
    int len = strlen(passwd);
    
    // 密文格式形如: $6$salt$hash
    for (i = 0; i < len; i++) {
        if (passwd[i] == '$') {
            count++;
            if (count == 3) { // 找到第三个 $，说明前面的 $id$salt$ 已经找完了
                break;
            }
        }
    }
    
    if (count == 3) {
        strncpy(setting, passwd, i); // 把 $id$salt$ 复制过去
        setting[i] = '\0';
    } else {
        setting[0] = '\0';
    }
}
void loginCheck1(user_info_t *user)
{
    printf("loginCheck1.\n");
    TLVHeader header;
    int ret;
    bzero(&header,sizeof(TLVHeader));
    struct spwd *sp=getspnam(user->username);
    if(sp==NULL)//用户名不存在
    {
        header.type=TASK_LOGIN_SECTION1_RESP_ERROR;
        header.length=0;
        ret=sendn(user->sockfd,&header,8);
        printf("check1 send %d bytes\n",ret);
        return;
    }
    //用户存在的情况下
    char setting[100]={0};
    //保存加密密文
    strcpy(user->encrypted,sp->sp_pwdp);
    getsetting(setting,sp->sp_pwdp);
    header.length = strlen(setting);
    header.type = TASK_LOGIN_SECTION1_RESP_OK;

    ret=sendn(user->sockfd,&header,sizeof(TLVHeader));
    if(header.length>0){
        ret = sendn(user->sockfd,setting,header.length);
    }
    printf("check1 send %d bytes.\n", ret);
}

void loginCheck2(user_info_t *user, const char *encrypted) {
    TLVHeader header;
       // 【新增两行：看看服务器收到的是什么，应该比对的是什么】
    printf("[DEBUG] 服务器收到客户端密文: [%s]\n", encrypted);
    printf("[DEBUG] 服务器影子文件密文: [%s]\n", user->encrypted);

    // 比较客户端传来的密文和 shadow 里的密文
    if (strcmp(user->encrypted, encrypted) == 0) {
        user->status = STATUS_LOGIN; // 登录成功
        header.type = TASK_LOGIN_SECTION2_RESP_OK;
        // 可以把用户的初始虚拟路径发过去
        header.length = strlen("/"); 
        sendn(user->sockfd, &header, sizeof(TLVHeader));
        sendn(user->sockfd, "/", header.length);
    } else {
        header.type = TASK_LOGIN_SECTION2_RESP_ERROR;
        header.length = 0;
        sendn(user->sockfd, &header, sizeof(TLVHeader));
    }
}