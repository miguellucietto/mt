#ifndef MT_MINIBUFFER_H
#define MT_MINIBUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MINIBUFFER_INACTIVE,
    MINIBUFFER_COMMAND,
    MINIBUFFER_SHELL,
    MINIBUFFER_FIND_FILE,
    MINIBUFFER_DIRED,
    MINIBUFFER_CREATE_FILE,
    MINIBUFFER_CREATE_DIRECTORY,
    MINIBUFFER_RENAME,
    MINIBUFFER_DELETE_CONFIRM,
    MINIBUFFER_ISEARCH,
    MINIBUFFER_QUERY_FIND,
    MINIBUFFER_QUERY_REPLACE,
    MINIBUFFER_QUERY_CONFIRM
} MinibufferMode;

typedef struct {
    MinibufferMode mode;
    char prompt[64];
    char input[1024];
    size_t length;
} Minibuffer;

void minibuffer_open(Minibuffer *minibuffer, MinibufferMode mode, const char *prompt);
void minibuffer_close(Minibuffer *minibuffer);
void minibuffer_insert(Minibuffer *minibuffer, const char *text);
void minibuffer_backspace(Minibuffer *minibuffer);

#endif
