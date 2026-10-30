#include "threadpool.h"

// 工人线程函数
void *worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    while (1) {
        pthread_mutex_lock(&pool->lock);
        while (pool->count == 0) {
            pthread_cond_wait(&pool->notify, &pool->lock); // 没活干就睡觉
        }
        
        // 拿任务
        Task *task = pool->head;
        pool->head = task->next;
        if (pool->head == NULL) pool->tail = NULL;
        pool->count--;
        pthread_mutex_unlock(&pool->lock);

        // 干活
        handle_client(task->client_socket);
        free(task);
    }
    return NULL;
}

void pool_init(ThreadPool *pool) {
    pool->head = NULL; pool->tail = NULL; pool->count = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);
    
    // 预创建 4 个打工人
    pthread_t tid;
    for (int i = 0; i < 4; i++) {
        pthread_create(&tid, NULL, worker_thread, pool);
        pthread_detach(tid); // 分离线程，让它自己回收
    }
    printf("[线程池] 初始化完成，创建了 4 个线程\n");
}

void pool_add_task(ThreadPool *pool, int client_socket) {
    Task *new_task = (Task *)malloc(sizeof(Task));
    new_task->client_socket = client_socket;
    new_task->next = NULL;

    pthread_mutex_lock(&pool->lock);
    if (pool->tail == NULL) {
        pool->head = pool->tail = new_task;
    } else {
        pool->tail->next = new_task;
        pool->tail = new_task;
    }
    pool->count++;
    pthread_cond_signal(&pool->notify); // 唤醒一个睡着的线程
    pthread_mutex_unlock(&pool->lock);
}