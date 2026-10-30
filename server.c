#include "protocol.h"

#define PORT 12345
#define REAL_ROOT "/tmp/cloud_root" // 你的真实根目录

// 辅助函数：根据虚拟路径拼真实路径
void get_real_path(const char *virtual_path, const char *value, char *real_path) {
    if (strcmp(virtual_path, "/") == 0) {
        sprintf(real_path, "%s/%s", REAL_ROOT, value);
    } else {
        sprintf(real_path, "%s%s/%s", REAL_ROOT, virtual_path, value);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    TLVHeader header;
    char value[4096] = {0};
    char response[4096] = {0};

    // 初始化 Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); exit(EXIT_FAILURE);
    }
    // 允许端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    
    printf("[服务端] 正在监听端口 %d...\n", PORT);
    printf("[服务端] 真实根目录设定为: %s\n", REAL_ROOT);
    fflush(stdout); // 强制刷新输出

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept"); exit(EXIT_FAILURE);
    }
    printf("[服务端] 客户端已连接！等待命令...\n");
    fflush(stdout);

    // 维护虚拟路径
    char virtual_path[1024] = "/"; 

    while (read(new_socket, &header, sizeof(TLVHeader)) > 0) {
        memset(value, 0, sizeof(value));
        memset(response, 0, sizeof(response));
        
        // 读取 Value (如果有的话)
        if (header.length > 0) {
            read(new_socket, value, header.length);
        }

        printf("[服务端日志] 收到 Type: %d, Length: %d, Value: %s\n", header.type, header.length, value);
        fflush(stdout);

        // ---------------- 处理 PWD ----------------
        if (header.type == CMD_PWD) {
            strcpy(response, virtual_path);
        } 
        // ---------------- 处理 CD ----------------
        else if (header.type == CMD_CD) {
            char temp_virtual[1024];
            strcpy(temp_virtual, virtual_path);

            if (strcmp(value, "..") == 0) {
                if (strcmp(virtual_path, "/") != 0) {
                    char *last_slash = strrchr(temp_virtual, '/');
                    if (last_slash != temp_virtual) *last_slash = '\0'; 
                    else strcpy(temp_virtual, "/");
                }
            } else {
                if (strcmp(temp_virtual, "/") != 0) strcat(temp_virtual, "/");
                strcat(temp_virtual, value);
            }

            char real_path[2048];
            if (strcmp(temp_virtual, "/") == 0) sprintf(real_path, "%s", REAL_ROOT);
            else sprintf(real_path, "%s%s", REAL_ROOT, temp_virtual);

            if (access(real_path, F_OK | R_OK | X_OK) == 0) {
                strcpy(virtual_path, temp_virtual);
                sprintf(response, "Success: 切换到 %s\n", virtual_path);
            } else {
                sprintf(response, "Error: 目录 '%s' 不存在!\n", value);
            }
        }
        // ---------------- 处理 LS ----------------
        else if (header.type == CMD_LS) {
            char real_path[2048];
            if (strcmp(virtual_path, "/") == 0) sprintf(real_path, "%s", REAL_ROOT);
            else sprintf(real_path, "%s%s", REAL_ROOT, virtual_path);

            DIR *dir = opendir(real_path);
            if (dir != NULL) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        strcat(response, entry->d_name);
                        strcat(response, "  ");
                    }
                }
                strcat(response, "\n");
                closedir(dir);
            } else {
                strcpy(response, "Error: 无法打开目录\n");
            }
        }
        // ---------------- 处理 REMOVE ----------------
        else if (header.type == CMD_REMOVE) {
            char real_path[2048];
            get_real_path(virtual_path, value, real_path);
            
            if (unlink(real_path) == 0) {
                strcpy(response, "Success: 文件已删除\n");
            } else {
                sprintf(response, "Error: 无法删除 '%s' (文件不存在或没权限)\n", value);
            }
        }
        else {
            strcpy(response, "Error: 未知命令\n");
        }

        // 统一发送回复
        TLVHeader res_header;
        res_header.type = header.type;
        res_header.length = strlen(response);
        write(new_socket, &res_header, sizeof(TLVHeader));
        write(new_socket, response, res_header.length);
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
