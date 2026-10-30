#ifndef STACK_H
#define STACK_H
#include <my_header.h>

#define MAX_DEPTH 100

typedef struct {
    char *items[MAX_DEPTH];
    int top;
} PathStack;

void init_stack(PathStack *s);
void push(PathStack *s, const char *dir);
void pop(PathStack *s);
void get_virtual_path(PathStack *s, char *out_buf);
void clear_stack(PathStack *s);

#endif