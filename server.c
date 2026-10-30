#include "protocol.h"
#include "threadpool.h"
#include "file_ops.h" // 引入业务层

#define PORT 12345

// 这个函数只负责收包、发包、分发，毫无具体业务代码
void handle_client(int client_socket) {
    TLVHeader header;
    char value[4096] = {0};
    char response[4096] = {0};
    
    // 为该客户端创建独立的上下文
    ClientContext ctx;
    context_init(&ctx);

    while (read(client_socket, &header, sizeof(TLVHeader)) > 0) {
        memset(value, 0, sizeof(value));
        memset(response, 0, sizeof(response));
        
        if (header.length > 0) read(client_socket, value, header.length);

        // 命令路由分发
        switch (header.type) {
            case CMD_PWD:    cmd_pwd(&ctx, value, response); break;
            case CMD_CD:     cmd_cd(&ctx, value, response); break;
            case CMD_LS:     cmd_ls(&ctx, value, response); break;
            case CMD_REMOVE: cmd_remove(&ctx, value, response); break;
            case CMD_PUTS:   cmd_puts(&ctx, value, response); break;
            case CMD_PUTS_DATA: 
                cmd_puts_data(&ctx, value, header.length); 
                continue; // 数据流直接continue，不回包
            case CMD_PUTS_END:  cmd_puts_end(&ctx, response); break;
             case CMD_GETS:
                cmd_gets(&ctx, value, response, client_socket);
                continue; // 因为 cmd_gets 内部已经把数据全发完了，这里直接跳过外面的统一发包
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
    close(client_socket);
}

int main() {
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
    return 0;
}