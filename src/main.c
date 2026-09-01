#include "editor.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    Editor editor;
    if (!editor_init(&editor, argc > 1 ? argv[1] : NULL)) {
        fprintf(stderr, "Não foi possível iniciar o editor: %s\n", SDL_GetError());
        return 1;
    }
    editor_run(&editor);
    editor_destroy(&editor);
    return 0;
}
