#include "stack.h"

void init_stack(PathStack *s) { s->top = -1; }

void push(PathStack *s, const char *dir) {
    if (s->top < MAX_DEPTH - 1) {
        s->top++;
        s->items[s->top] = strdup(dir); // 记得 free
    }
}

void pop(PathStack *s) {
    if (s->top >= 0) {
        free(s->items[s->top]);
        s->top--;
    }
}

void get_virtual_path(PathStack *s, char *out_buf) {
    if (s->top == -1) { strcpy(out_buf, "/"); return; }
    strcpy(out_buf, "");
    for (int i = 0; i <= s->top; i++) {
        strcat(out_buf, "/");
        strcat(out_buf, s->items[i]);
    }
}

void clear_stack(PathStack *s) {
    while (s->top >= 0) pop(s);
}