#ifndef MT_FILE_CONTROLLER_H
#define MT_FILE_CONTROLLER_H

#include "buffer.h"
#include "minibuffer.h"

#include <stdbool.h>

struct Editor;

typedef struct {
    char pending_open_path[MT_PATH_SIZE];
} FileState;

bool file_register_commands(struct Editor *editor);
void file_request_quit(struct Editor *editor);
bool file_submit(struct Editor *editor, MinibufferMode mode, const char *value);
void file_open(struct Editor *editor, const char *path);

#endif
