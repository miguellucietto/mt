#ifndef MT_EDITING_H
#define MT_EDITING_H

#include "editor.h"

#include <stdbool.h>

/** Registers the complete native editing and navigation command set. */
bool editing_register_commands(Editor *editor);

#endif
