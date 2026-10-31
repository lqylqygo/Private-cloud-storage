# 定义编译器
CC = gcc
# 编译选项：-Wall 开启所有警告，-g 加入调试信息，-pthread 链接线程库
CFLAGS = -Wall -g -pthread

# 定义目标可执行文件
TARGETS = server client

# 服务端需要的所有目标文件（【修改】加入 protocol.o 和 user.o）
SERVER_OBJS = server.o threadpool.o stack.o file_ops.o user.o protocol.o

# 默认目标：敲 make 时默认编译所有
all: $(TARGETS)

# 服务端（【修改】链接时加入 -lcrypt）
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lcrypt

# 客户端（【修改】因为客户端可能也用到了 protocol.c 的函数，所以把 protocol.o 加进来）
client: client.o protocol.o
	$(CC) $(CFLAGS) -o $@ $^  -lcrypt

# 编译所有的 .c 文件为 .o 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 自动推导头文件依赖
server.o: server.c protocol.h threadpool.h file_ops.h user.h
threadpool.o: threadpool.c threadpool.h 
stack.o: stack.c stack.h 
file_ops.o: file_ops.c file_ops.h protocol.h stack.h
client.o: client.c protocol.h 
user.o: user.c user.h protocol.h
protocol.o: protocol.c protocol.h

# 清理编译生成的中间文件和可执行文件
clean:
	rm -f *.o $(TARGETS)
