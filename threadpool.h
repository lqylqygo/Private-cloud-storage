#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <my_header.h>

// 定义任务节点（其实就是一个客户端 socket）
typedef struct Task {
    int client_socket;
    struct Task *next;
} Task;

typedef struct {
    Task *head;
    Task *tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t notify;
} ThreadPool;

void pool_init(ThreadPool *pool);
void pool_add_task(ThreadPool *pool, int client_socket);

// 这个函数需要在 server.c 里实现，线程池会调用它
void handle_client(int client_socket); 

#endif