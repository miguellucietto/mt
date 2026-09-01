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

/** Registers incremental-search and query-replace commands. */
bool search_register_commands(struct Editor *editor);
/** Handles search minibuffer modes and reports whether mode was handled. */
bool search_submit(struct Editor *editor, MinibufferMode mode, const char *value);
/** Consumes query-replace confirmation keys while confirmation is active. */
bool search_handle_confirmation(struct Editor *editor, const SDL_Event *event);
/** Cancels incremental search and restores its original cursor position. */
void search_cancel(struct Editor *editor);
/** Selects the next query occurrence, wrapping through the document. */
void search_next(struct Editor *editor);
/** Recomputes the incremental-search match from current minibuffer input. */
void search_update(struct Editor *editor);

#endif
