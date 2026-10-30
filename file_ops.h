#ifndef FILE_OPS_H
#define FILE_OPS_H
#include <my_header.h>
#include "stack.h"

// 定义一个上下文，保存一个客户端的全部状态
typedef struct {
    PathStack path_stack;   // 虚拟路径栈
    FILE *upload_fp;        // 当前上传文件句柄
} ClientContext;

// 初始化上下文
void context_init(ClientContext *ctx);
// 清理上下文（断开连接时调用）
void context_destroy(ClientContext *ctx);

// 具体的命令处理函数
void cmd_pwd(ClientContext *ctx, const char *arg, char *response);
void cmd_cd(ClientContext *ctx, const char *arg, char *response);
void cmd_ls(ClientContext *ctx, const char *arg, char *response);
void cmd_remove(ClientContext *ctx, const char *arg, char *response);
void cmd_puts(ClientContext *ctx, const char *arg, char *response);
void cmd_puts_data(ClientContext *ctx, const char *data, int length);
void cmd_puts_end(ClientContext *ctx, char *response);
void cmd_gets(ClientContext *ctx, const char *arg, char *response, int sock);
void cmd_gets_data(ClientContext *ctx, int sock);
#endif