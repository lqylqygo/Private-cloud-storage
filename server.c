#include "protocol.h"
#include "threadpool.h"
#include "file_ops.h" //业务层
#include "user.h"     // 引入用户验证头文件


#define PORT 12345

// 这个函数只负责收包、发包、分发，毫无具体业务代码
void handle_client(int client_socket) {
    TLVHeader header;
    char value[4096] = {0};
    char response[4096] = {0};
    
    // 为该客户端创建独立的上下文
    ClientContext ctx;
    context_init(&ctx);

    // 为当前连接创建独立的用户状态
    user_info_t user;
    user.sockfd = client_socket;
    user.status = STATUS_LOGOFF; // 初始状态为未登录
    strcpy(user.pwd, "/");
    memset(user.username, 0, sizeof(user.username));
    memset(user.encrypted, 0, sizeof(user.encrypted));
    syslog(LOG_INFO, "[线程池] 新客户端连接，sockfd=%d", client_socket);
    while (read(client_socket, &header, sizeof(TLVHeader)) > 0) {
        memset(value, 0, sizeof(value));
        memset(response, 0, sizeof(response));
        
        if (header.length > 0) read(client_socket, value, header.length);

        // ================= 【新增：登录拦截逻辑】 =================
        if (user.status == STATUS_LOGOFF) {
            switch (header.type) {
                case TASK_LOGIN_SECTION1:
                    // 阶段1：收到用户名
                    strncpy(user.username, value, sizeof(user.username) - 1);
                    loginCheck1(&user); // 调用 user.c 里的函数
                    break;
                case TASK_LOGIN_SECTION2:
                    // 阶段2：收到密文
                    loginCheck2(&user, value); // 调用 user.c 里的函数
                    break;
                default:
                    // 未登录状态下发了别的命令
                    strcpy(response, "Error: Please login first.\n");
                    TLVHeader res = {CMD_INVALID, strlen(response)};
                    write(client_socket, &res, sizeof(TLVHeader));
                    write(client_socket, response, res.length);
                    break;
            }
            syslog(LOG_INFO,"%s用户登录成功",user.username);
            continue; // 登录阶段处理完毕，直接跳过下面的业务分发
        }
        // =========================================================
        // --- 以下为已登录后的业务逻辑，保持你原来的逻辑不变 ---
        switch (header.type) {
            case CMD_PWD:
             syslog(LOG_INFO, "User [%s] use pwd tochecked current path.", user.username);    
            cmd_pwd(&ctx, value, response); break;
            case CMD_CD:    
            syslog(LOG_INFO, "User [%s] use cd to change path to %s.", user.username, value);
            cmd_cd(&ctx, value, response); break;
            case CMD_LS:     
            syslog(LOG_INFO, "User [%s] use ls to list files.", user.username);
            cmd_ls(&ctx, value, response); break;
            case CMD_REMOVE:
            syslog(LOG_WARNING, "User [%s] use remove to delete file %s.", user.username, value);
            cmd_remove(&ctx, value, response); break;
            case CMD_PUTS:   cmd_puts(&ctx, value, response); break;
            case CMD_PUTS_DATA: 
                cmd_puts_data(&ctx, value, header.length); 
                continue; 
            case CMD_PUTS_END:  cmd_puts_end(&ctx, response); break;
            case CMD_GETS:
                cmd_gets(&ctx, value, response, client_socket);
                continue; 
            default: 
                strcpy(response, "Error: Unknown Command\n"); 
                break;
        }

        // 发送响应
        TLVHeader res = {header.type, strlen(response)};
        write(client_socket, &res, sizeof(TLVHeader));
        write(client_socket, response, res.length);
    }
    
    context_destroy(&ctx);
    syslog(LOG_INFO, "[线程池] 客户端断开连接，sockfd=%d", client_socket);
    close(client_socket);
}

// main 保持不变
int main() {
    openlog("CloudDisk_Server",LOG_PID|LOG_CONS,LOG_USER);
    // 1. 初始化线程池
    ThreadPool pool;
    pool_init(&pool);

    // 2. 初始化网络
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); // PORT 是 8888

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen failed"); exit(EXIT_FAILURE);
    }
    printf("[主线程] 服务端监听 %d...\n", PORT);

    // 3. 主线程只负责接客并把 socket 扔进线程池
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        // 这里接收客户端
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }
        printf("[主线程] 新客户端连入，丢入线程池\n");
        pool_add_task(&pool, client_socket); 
    }
    closelog();
    return 0;
}
