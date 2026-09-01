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
    MINIBUFFER_REPLACE_BUFFER_CONFIRM,
    MINIBUFFER_QUIT_CONFIRM,
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

/** Starts a prompt session with empty input in the requested mode. */
void minibuffer_open(Minibuffer *minibuffer, MinibufferMode mode, const char *prompt);
/** Ends the current prompt and clears its input. */
void minibuffer_close(Minibuffer *minibuffer);
/** Appends UTF-8 text when the bounded input buffer has sufficient space. */
void minibuffer_insert(Minibuffer *minibuffer, const char *text);
/** Removes the final UTF-8 codepoint from minibuffer input. */
void minibuffer_backspace(Minibuffer *minibuffer);

#endif
