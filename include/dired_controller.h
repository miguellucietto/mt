#ifndef MT_DIRED_CONTROLLER_H
#define MT_DIRED_CONTROLLER_H

#include "buffer.h"
#include "minibuffer.h"

#include <SDL3/SDL.h>
#include <stdbool.h>

struct Editor;

typedef struct {
    char pending_path[MT_PATH_SIZE];
} DiredState;

bool dired_register_commands(struct Editor *editor);
bool dired_submit(struct Editor *editor, MinibufferMode mode, const char *value);
bool dired_handle_event(struct Editor *editor, const SDL_Event *event);
void dired_open_selected(struct Editor *editor);

#endif
