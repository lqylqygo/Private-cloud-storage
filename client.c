#include "protocol.h"

#define SERVER_IP "127.0.0.1"
#define PORT 12345

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    TLVHeader header;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket 创建失败\n"); return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nIP 地址无效\n"); return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n连接失败\n"); return -1;
    }
    printf("成功连接到服务端！(支持命令: pwd, cd, ls, remove)\n");

    char input[1024];
    while (1) {
        printf("> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; // 去除换行符

        if (strlen(input) == 0) continue;

        // 复制一份用于 strtok 分割，因为 strtok 会破坏原字符串
        char input_copy[1024];
        strcpy(input_copy, input);

        char *cmd = strtok(input_copy, " ");
        char *arg = strtok(NULL, " ");

        if (cmd == NULL) continue;

        // --- 解析并封装命令 ---
        if (strcmp(cmd, "pwd") == 0) {
            header.type = CMD_PWD;
            header.length = 0;
        } 
        else if (strcmp(cmd, "ls") == 0) {
            header.type = CMD_LS;
            header.length = 0;
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (arg == NULL) { printf("用法: cd <目录名>\n"); continue; }
            header.type = CMD_CD;
            header.length = strlen(arg);
            strcpy(input, arg); // 把目录名暂存到 input 里备用
        } 
        else if (strcmp(cmd, "remove") == 0) {
            if (arg == NULL) { printf("用法: remove <文件名>\n"); continue; }
            header.type = CMD_REMOVE;
            header.length = strlen(arg);
            strcpy(input, arg);
        }
        else {
            printf("未知命令！(支持: pwd, cd, ls, remove)\n");
            continue;
        }

        // 发送头部
        write(sock, &header, sizeof(TLVHeader));
        // 如果有参数，发送参数
        if (header.length > 0) {
            write(sock, input, header.length);
        }

        // --- 接收服务端响应 ---
        TLVHeader res_header;
        if (read(sock, &res_header, sizeof(TLVHeader)) > 0) {
            char response[4096] = {0};
            read(sock, response, res_header.length);
            printf("%s", response);
        }
    }

    close(sock);
    return 0;
}
