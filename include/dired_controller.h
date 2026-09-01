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

/** Registers every command owned by the Dired controller. */
bool dired_register_commands(struct Editor *editor);
/** Handles Dired minibuffer modes and reports whether mode belonged to Dired. */
bool dired_submit(struct Editor *editor, MinibufferMode mode, const char *value);
/** Handles Dired-specific events and reports whether the event was consumed. */
bool dired_handle_event(struct Editor *editor, const SDL_Event *event);
/** Opens or enters the directory entry under the active buffer cursor. */
void dired_open_selected(struct Editor *editor);

#endif
