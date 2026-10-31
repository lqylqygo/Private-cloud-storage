#include "protocol.h"
#include "threadpool.h"
#include "file_ops.h" //业务层
#include "user.h"     // 引入用户验证头文件


#define PORT 12345

// ================= 时间函数 =================
// 传入一个 buffer，把当前时间格式化进去，这样每次都是独立的，线程安全
void get_current_time(char *buf, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    // 格式化为: YYYY-MM-DD HH:MM:SS
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}
// ===================================================================

void handle_client(int client_socket) {
    TLVHeader header;
    char value[4096] = {0};
    char response[4096] = {0};
    
    ClientContext ctx;
    context_init(&ctx);

    user_info_t user;
    user.sockfd = client_socket;
    user.status = STATUS_LOGOFF; 
    strcpy(user.pwd, "/");
    memset(user.username, 0, sizeof(user.username));
    memset(user.encrypted, 0, sizeof(user.encrypted));

    // 加上时间
    char time_str[64] = {0};
    get_current_time(time_str, sizeof(time_str));
    syslog(LOG_INFO, "[%s] [线程池] 新客户端连接，sockfd=%d", time_str, client_socket);

    while (read(client_socket, &header, sizeof(TLVHeader)) > 0) {
        memset(value, 0, sizeof(value));
        memset(response, 0, sizeof(response));
        
        if (header.length > 0) read(client_socket, value, header.length);

        // ================= 登录拦截逻辑 =================
        if (user.status == STATUS_LOGOFF) {
            switch (header.type) {
                case TASK_LOGIN_SECTION1:
                    strncpy(user.username, value, sizeof(user.username) - 1);
                    loginCheck1(&user); 
                    break;
                case TASK_LOGIN_SECTION2:
                    loginCheck2(&user, value); 
                    // 【修改】这里之前无论成败都会打成功，我帮你改到判断里面了
                    if (user.status == STATUS_LOGIN) {
                        get_current_time(time_str, sizeof(time_str));
                        syslog(LOG_INFO, "[%s] 用户 [%s] 登录成功", time_str, user.username);
                    } else {
                        get_current_time(time_str, sizeof(time_str));
                        syslog(LOG_WARNING, "[%s] 用户 [%s] 密码错误", time_str, user.username);
                    }
                    break;
                default:
                    strcpy(response, "Error: Please login first.\n");
                    TLVHeader res = {CMD_INVALID, strlen(response)};
                    write(client_socket, &res, sizeof(TLVHeader));
                    write(client_socket, response, res.length);
                    break;
            }
            continue; 
        }
        
        // --- 以下为已登录后的业务逻辑 ---
        switch (header.type) {
            case CMD_PWD:
                get_current_time(time_str, sizeof(time_str)); // 每次打印前获取一下最新时间
                syslog(LOG_INFO, "[%s] User [%s] checked current path.", time_str, user.username);    
                cmd_pwd(&ctx, value, response); 
                break;
            case CMD_CD:    
                get_current_time(time_str, sizeof(time_str));
                syslog(LOG_INFO, "[%s] User [%s] changed path to [%s].", time_str, user.username, value);
                cmd_cd(&ctx, value, response); 
                break;
            case CMD_LS:     
                get_current_time(time_str, sizeof(time_str));
                syslog(LOG_INFO, "[%s] User [%s] listed files.", time_str, user.username);
                cmd_ls(&ctx, value, response); 
                break;
            case CMD_REMOVE:
                get_current_time(time_str, sizeof(time_str));
                syslog(LOG_WARNING, "[%s] User [%s] deleted file [%s].", time_str, user.username, value);
                cmd_remove(&ctx, value, response); 
                break;
            case CMD_PUTS:   
                get_current_time(time_str, sizeof(time_str));
                syslog(LOG_INFO, "[%s] User [%s] started uploading file [%s].", time_str, user.username, value);
                cmd_puts(&ctx, value, response); 
                break;
            case CMD_PUTS_DATA: 
                cmd_puts_data(&ctx, value, header.length); 
                continue; 
            case CMD_PUTS_END:  
                get_current_time(time_str, sizeof(time_str));
                syslog(LOG_INFO, "[%s] User [%s] finished uploading.", time_str, user.username);
                cmd_puts_end(&ctx, response); 
                break;
            case CMD_GETS:
                get_current_time(time_str, sizeof(time_str));
                syslog(LOG_INFO, "[%s] User [%s] started downloading file [%s].", time_str, user.username, value);
                cmd_gets(&ctx, value, response, client_socket);
                continue; 
            default: 
                strcpy(response, "Error: Unknown Command\n"); 
                  syslog(LOG_WARNING, "[%s] User [%s] sent an UNKNOWN command type: %d", time_str, user.username, header.type);
                break;
        }

        TLVHeader res = {header.type, strlen(response)};
        write(client_socket, &res, sizeof(TLVHeader));
        write(client_socket, response, res.length);
    }
    
    context_destroy(&ctx);
    // 断开连接加上时间
    get_current_time(time_str, sizeof(time_str));
    syslog(LOG_INFO, "[%s] [线程池] 客户端断开连接，sockfd=%d", time_str, client_socket);
    close(client_socket);
}

int main() {
    openlog("CloudDisk_Server", LOG_PID | LOG_CONS, LOG_USER);
    ThreadPool pool;
    pool_init(&pool);

    // ... (中间的 socket 初始化代码保持原样) ...
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen failed"); exit(EXIT_FAILURE);
    }
    printf("[主线程] 服务端监听 %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
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