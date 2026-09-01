#ifndef MT_SEARCH_H
#define MT_SEARCH_H

#include "minibuffer.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>

struct Editor;

typedef struct {
    char query[1024];
    char replacement[1024];
    size_t origin;
} SearchState;

bool search_register_commands(struct Editor *editor);
bool search_submit(struct Editor *editor, MinibufferMode mode, const char *value);
bool search_handle_confirmation(struct Editor *editor, const SDL_Event *event);
void search_cancel(struct Editor *editor);
void search_next(struct Editor *editor);
void search_update(struct Editor *editor);

#endif
