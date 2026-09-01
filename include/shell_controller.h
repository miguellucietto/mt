#ifndef MT_SHELL_CONTROLLER_H
#define MT_SHELL_CONTROLLER_H

#include "minibuffer.h"

#include <stdbool.h>

struct Editor;

/** Registers the shell-command prompt command. */
bool shell_register_commands(struct Editor *editor);
/** Handles shell minibuffer submission and presents captured output. */
bool shell_submit(struct Editor *editor, MinibufferMode mode, const char *value);

#endif
