#include "file_ops.h"
#include "protocol.h" // 为了拿到 REAL_ROOT (或者你可以在 my_header.h 定义)

#define REAL_ROOT "/tmp/cloud_root"

void context_init(ClientContext *ctx) {
    init_stack(&ctx->path_stack);
    ctx->upload_fp = NULL;
}

void context_destroy(ClientContext *ctx) {
    clear_stack(&ctx->path_stack);
    if (ctx->upload_fp) {
        fclose(ctx->upload_fp);
        ctx->upload_fp = NULL;
    }
}

// 辅助函数：获取真实物理路径
static void get_real_path(ClientContext *ctx, const char *filename, char *real_path) {
    char v_path[1024];
    get_virtual_path(&ctx->path_stack, v_path);
    
    if (strcmp(v_path, "/") == 0) {
        sprintf(real_path, "%s/%s", REAL_ROOT, filename);
    } else {
        sprintf(real_path, "%s%s/%s", REAL_ROOT, v_path, filename);
    }
}

void cmd_pwd(ClientContext *ctx, const char *arg, char *response) {
    get_virtual_path(&ctx->path_stack, response);
}

void cmd_cd(ClientContext *ctx, const char *arg, char *response) {
    if (arg == NULL || strlen(arg) == 0) { strcpy(response, "Error: 缺少目录名\n"); return; }
    
    if (strcmp(arg, "..") == 0) {
        pop(&ctx->path_stack);
        strcpy(response, "Success\n");
        return;
    }
    
    // 模拟进入后的路径进行判断
    char temp_v[1024], real_p[2048];
    get_virtual_path(&ctx->path_stack, temp_v);
    if (strcmp(temp_v, "/") != 0) strcat(temp_v, "/");
    strcat(temp_v, arg);
    sprintf(real_p, "%s%s", REAL_ROOT, temp_v);

    if (access(real_p, F_OK | R_OK | X_OK) == 0) {
        push(&ctx->path_stack, arg);
        strcpy(response, "Success\n");
    } else {
        strcpy(response, "Error: 目录不存在\n");
    }
}

void cmd_ls(ClientContext *ctx, const char *arg, char *response) {
    char v_path[1024], real_p[2048];
    get_virtual_path(&ctx->path_stack, v_path);
    sprintf(real_p, "%s%s", REAL_ROOT, strcmp(v_path, "/") == 0 ? "" : v_path);
    
    DIR *dir = opendir(real_p);
    printf("[调试] 客户端请求 ls, 拼接的真实路径是: %s\n", real_p);
fflush(stdout);
    if (dir) {
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

void cmd_remove(ClientContext *ctx, const char *arg, char *response) {
    if (arg == NULL) { strcpy(response, "Error: 缺少文件名\n"); return; }
    char real_p[2048];
    get_real_path(ctx, arg, real_p);
    if (unlink(real_p) == 0) strcpy(response, "Success: Removed\n");
    else strcpy(response, "Error: Remove failed\n");
}

void cmd_puts(ClientContext *ctx, const char *arg, char *response) {
     if (arg == NULL) { strcpy(response, "Error: 缺少文件名\n"); return; }
    char real_p[2048];
    get_real_path(ctx, arg, real_p);
    
    printf("[调试] 准备创建文件: %s\n", real_p);
    
    ctx->upload_fp = fopen(real_p, "wb");
    if (ctx->upload_fp) {
        printf("[调试] fopen 成功！\n"); // <--- 加上这句
        strcpy(response, "Ready\n");
    } else {
        printf("[调试] fopen 失败！原因: %s\n", strerror(errno)); // <--- 加上这句
        strcpy(response, "Error: Cannot create file\n");
    }
}

void cmd_puts_data(ClientContext *ctx, const char *data, int length) {
    if (ctx->upload_fp) {
        fwrite(data, 1, length, ctx->upload_fp);
    }
}

void cmd_puts_end(ClientContext *ctx, char *response) {
    if (ctx->upload_fp) { 
        fclose(ctx->upload_fp); 
        ctx->upload_fp = NULL; 
    }
    strcpy(response, "Upload Complete\n");
}
void cmd_gets(ClientContext *ctx, const char *arg, char *response, int sock) {
    if (arg == NULL) { strcpy(response, "Error: 缺少文件名\n"); return; }
    
    char real_p[2048];
    get_real_path(ctx, arg, real_p);
    
    FILE *fp = fopen(real_p, "rb"); // 二进制读
    if (!fp) {
        strcpy(response, "Error: File not found\n");
        return;
    }

    // 先给客户端回一个 Ready 响应
    strcpy(response, "Ready\n");
    TLVHeader res = {CMD_GETS, strlen(response)};
    write(sock, &res, sizeof(TLVHeader));
    write(sock, response, res.length);

    // 开始分块读取并发送
    char file_buf[4096];
    int bytes_read;
    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
        TLVHeader data_header;
        data_header.type = CMD_GETS_DATA;
        data_header.length = bytes_read;
        write(sock, &data_header, sizeof(TLVHeader));
        write(sock, file_buf, bytes_read);
    }

    // 发送结束标记
    TLVHeader end_header = {CMD_GETS_END, 0};
    write(sock, &end_header, sizeof(TLVHeader));
    
    fclose(fp);
}