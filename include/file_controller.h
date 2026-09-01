#ifndef MT_FILE_CONTROLLER_H
#define MT_FILE_CONTROLLER_H

#include "buffer.h"
#include "minibuffer.h"

#include <stdbool.h>

struct Editor;

typedef struct {
    char pending_open_path[MT_PATH_SIZE];
} FileState;

/** Registers file-opening, saving, and quit commands. */
bool file_register_commands(struct Editor *editor);
/** Stops immediately or requests confirmation when modified buffers exist. */
void file_request_quit(struct Editor *editor);
/** Handles file-related minibuffer modes and reports whether mode was handled. */
bool file_submit(struct Editor *editor, MinibufferMode mode, const char *value);
/** Opens a file or directory, requesting replacement confirmation if required. */
void file_open(struct Editor *editor, const char *path);

#endif
