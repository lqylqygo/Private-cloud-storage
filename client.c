#include "protocol.h"

#define SERVER_IP "127.0.0.1" // 如果服务端在别的机器上，改成对应的IP
#define PORT 12345
void userLogin1(int sockfd, char *setting_buf) {
    TLVHeader header;
    char user[128] = {0};
    
    while (1) {
        printf("Username: ");
        fgets(user, sizeof(user), stdin);
        user[strcspn(user, "\n")] = 0; // 去除换行

        // 发送用户名
        header.type = TASK_LOGIN_SECTION1;
        header.length = strlen(user);
        write(sockfd, &header, sizeof(TLVHeader));
        write(sockfd, user, header.length);

        // 接收服务器的响应
        TLVHeader res_header;
        read(sockfd, &res_header, sizeof(TLVHeader));
        
        if (res_header.type == TASK_LOGIN_SECTION1_RESP_ERROR) {
            printf("用户不存在，请重新输入。\n");
            continue; // 用户名不对，重新输入
        }

        // 用户名正确，接收 setting，存入传入的 setting_buf 中
        if (res_header.length > 0) {
            read(sockfd, setting_buf, res_header.length);
            setting_buf[res_header.length] = '\0'; // 加上字符串结束符
        }
        break; // 第一阶段成功，退出循环
    }
}
// 第二阶段：使用 setting_buf 加密密码，发送密文
void userLogin2(int sockfd, const char *setting_buf) {
    TLVHeader header;
    char pwd[128] = {0};
    
    while (1) {
        printf("Password: "); // 明文回显
        fgets(pwd, sizeof(pwd), stdin);
        pwd[strcspn(pwd, "\n")] = 0; // 去除换行

        printf("[DEBUG] 你输入的原始密码是: [%s], 长度: %ld\n", pwd, strlen(pwd));
        printf("[DEBUG] 服务器发来的 setting 是: [%s]\n", setting_buf);
        // 使用第一阶段拿到的 setting_buf 进行加密
        char *encrypted = crypt(pwd, setting_buf); 

        printf("[DEBUG] 客户端生成的密文是: [%s]\n", encrypted);
        // 发送密文
        header.type = TASK_LOGIN_SECTION2;
        header.length = strlen(encrypted);
        write(sockfd, &header, sizeof(TLVHeader));
        write(sockfd, encrypted, header.length);

        // 接收验证结果
        TLVHeader res_header;
        read(sockfd, &res_header, sizeof(TLVHeader));
        
        if (res_header.type == TASK_LOGIN_SECTION2_RESP_OK) {
            if (res_header.length > 0) {
                char path[1024] = {0};
                read(sockfd, path, res_header.length);
                printf("Login Success! Current path: %s\n", path);
            } else {
                printf("Login Success!\n");
            }
            break; // 登录成功，退出循环
        } else {
            printf("Password incorrect, try again.\n");
            continue; // 密码错误，重新输入
        }
    }
}
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    TLVHeader header;

    // 1. 创建 Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket 创建失败\n"); 
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 转换 IP 地址
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nIP 地址无效\n"); 
        return -1;
    }

    // 2. 连接服务端
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n连接失败，请确认服务端是否已启动\n"); 
        return -1;
    }
    
    printf("=========================================\n");
    printf("  成功连接到服务端！\n");
    // ================= 【修改：调用拆分后的两个函数】 =================
    char setting[128] = {0}; // 用普通数组存 setting 即可
    
    userLogin1(sock, setting); // 先执行第一段，把 setting 拿回来
    userLogin2(sock, setting); // 再执行第二段，用 setting 加密密码
    // =================================================================
    printf("  支持命令: pwd, cd <目录>, ls, remove <文件>\n");
    printf("            puts <本地文件>, gets <服务器文件>\n");
    printf("=========================================\n");

    char input[1024];
    
    // 3. 主循环：读取用户输入
    while (1) {
        printf("> ");
        fgets(input, sizeof(input), stdin);
        
        // 去掉末尾的换行符
        input[strcspn(input, "\n")] = 0; 

        // 处理空输入
        if (strlen(input) == 0) continue;

        // 复制一份用于 strtok 分割（因为 strtok 会破坏原字符串）
        char input_copy[1024];
        strcpy(input_copy, input);

        // 解析命令和参数
        char *cmd = strtok(input_copy, " ");
        char *arg = strtok(NULL, " ");

        if (cmd == NULL) continue;

        // ---------------- 1. 解析并发送命令 ----------------
        if (strcmp(cmd, "pwd") == 0) {
            header.type = CMD_PWD;
            header.length = 0;
            write(sock, &header, sizeof(TLVHeader));
        } 
        else if (strcmp(cmd, "ls") == 0) {
            header.type = CMD_LS;
            header.length = 0;
            write(sock, &header, sizeof(TLVHeader));
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (arg == NULL) { printf("用法: cd <目录名>\n"); continue; }
            header.type = CMD_CD;
            header.length = strlen(arg);
            write(sock, &header, sizeof(TLVHeader));
            write(sock, arg, header.length);
        } 
        else if (strcmp(cmd, "remove") == 0) {
            if (arg == NULL) { printf("用法: remove <文件名>\n"); continue; }
            header.type = CMD_REMOVE;
            header.length = strlen(arg);
            write(sock, &header, sizeof(TLVHeader));
            write(sock, arg, header.length);
        }
        
        // ---------------- 2. 处理上传 (puts) ----------------
        else if (strcmp(cmd, "puts") == 0) {
            if (arg == NULL) { printf("用法: puts <本地文件名>\n"); continue; }
            
            FILE *fp = fopen(arg, "rb"); // 二进制读
            if (!fp) {
                printf("错误: 找不到本地文件 %s\n", arg);
                continue;
            }

            // 发送上传请求和文件名
            header.type = CMD_PUTS;
            header.length = strlen(arg);
            write(sock, &header, sizeof(TLVHeader));
            write(sock, arg, header.length);

            // 接收服务端响应 (Ready 或 Error)
            TLVHeader res_header;
            char res_buf[1024] = {0};
            read(sock, &res_header, sizeof(TLVHeader));
            read(sock, res_buf, res_header.length);
            
            if (strncmp(res_buf, "Error", 5) == 0) {
                printf("%s", res_buf);
                fclose(fp);
                continue;
            }

            // 分块发送文件数据
            char file_buf[4096];
            int bytes_read;
            while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
                header.type = CMD_PUTS_DATA;
                header.length = bytes_read;
                write(sock, &header, sizeof(TLVHeader));
                write(sock, file_buf, bytes_read);
            }

            // 发送结束标记
            header.type = CMD_PUTS_END;
            header.length = 0;
            write(sock, &header, sizeof(TLVHeader));

            // 接收最终的 "Upload Complete"
            read(sock, &res_header, sizeof(TLVHeader));
            read(sock, res_buf, res_header.length);
            printf("%s", res_buf);

            fclose(fp);
            continue; // 上传流程结束，跳过后面的统一接收
        }
        
        // ---------------- 3. 处理下载 (gets) ----------------
        else if (strcmp(cmd, "gets") == 0) {
            if (arg == NULL) { printf("用法: gets <服务器文件名>\n"); continue; }
            
            // 发送下载请求和文件名
            header.type = CMD_GETS;
            header.length = strlen(arg);
            write(sock, &header, sizeof(TLVHeader));
            write(sock, arg, header.length);

            // 接收服务端响应 (Ready 或 Error)
            TLVHeader res_header;
            char res_buf[1024] = {0};
            read(sock, &res_header, sizeof(TLVHeader));
            read(sock, res_buf, res_header.length);
            
            if (strncmp(res_buf, "Error", 5) == 0) {
                printf("%s", res_buf);
                continue;
            }
            
            // 为了防止覆盖本地文件，加个 download_ 前缀
            char local_filename[1024];
            sprintf(local_filename, "download_%s", arg);
            FILE *fp = fopen(local_filename, "wb"); // 二进制写
            
            if (!fp) {
                printf("错误: 无法在本地创建文件 %s\n", local_filename);
                continue;
            }

            // 循环接收数据块
            while (1) {
                TLVHeader data_header;
                read(sock, &data_header, sizeof(TLVHeader));
                
                if (data_header.type == CMD_GETS_END) {
                    break; // 接收完毕
                }
                
                if (data_header.type == CMD_GETS_DATA) {
                    char file_buf[4096] = {0};
                    read(sock, file_buf, data_header.length);
                    fwrite(file_buf, 1, data_header.length, fp); // 写入本地
                }
            }
            
            fclose(fp);
            printf("下载完成！已保存为 %s\n", local_filename);
            continue; // 下载流程结束，跳过后面的统一接收
        }
        
        // ---------------- 未知命令 ----------------
        else {
            printf("未知命令！请重新输入\n");
            continue;
        }

        // ---------------- 统一接收普通命令的响应 (pwd, cd, ls, remove) ----------------
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
