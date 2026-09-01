#ifndef MT_SHELL_CONTROLLER_H
#define MT_SHELL_CONTROLLER_H

#include "minibuffer.h"

#include <stdbool.h>

struct Editor;

bool shell_register_commands(struct Editor *editor);
bool shell_submit(struct Editor *editor, MinibufferMode mode, const char *value);

#endif
