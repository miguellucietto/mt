#include "editor.h"
#include <stdio.h>

/* Initializes the editor, runs its event loop, and returns a process-level status. */
int main(int argc, char **argv)
{
    Editor editor;
    if (!editor_init(&editor, argc > 1 ? argv[1] : NULL)) {
        const char *reason = editor.message[0] ? editor.message : SDL_GetError();
        fprintf(stderr, "Unable to start mt: %s\n", reason);
        return 1;
    }
    editor_run(&editor);
    editor_destroy(&editor);
    return 0;
}
