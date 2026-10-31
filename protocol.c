#include"protocol.h"


int sendn(int sockfd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *data = (const char *)buf;

    while (total_sent < len) {
        ssize_t sent = send(sockfd, data + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1; // 发送失败
        }
        total_sent += (size_t)sent;
    }
    return (int)total_sent;
}