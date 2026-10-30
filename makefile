# 定义编译器
CC = gcc
# 编译选项：-Wall 开启所有警告，-g 加入调试信息(方便vscode调试)，-pthread 链接线程库
CFLAGS = -Wall -g -pthread

# 定义目标可执行文件
TARGETS = server client

# 服务端需要的所有目标文件（把 .c 换成 .o）
SERVER_OBJS = server.o threadpool.o stack.o file_ops.o

# 默认目标：敲 make 时默认编译所有
all: $(TARGETS)

# 服务端
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 客户端
client: client.o
	$(CC) $(CFLAGS) -o $@ $^

# 编译所有的 .c 文件为 .o 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 自动推导头文件依赖（如果头文件改了，相关的 .c 会自动重编）
server.o: server.c protocol.h threadpool.h file_ops.h 
threadpool.o: threadpool.c threadpool.h 
stack.o: stack.c stack.h 
file_ops.o: file_ops.c file_ops.h protocol.h stack.h
client.o: client.c protocol.h 

# 清理编译生成的中间文件和可执行文件
clean:
	rm -f *.o $(TARGETS)
