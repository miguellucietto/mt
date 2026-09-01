#include "editor.h"

#include <stdio.h>

static void hello(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = buffers_open_text(&editor->buffers, "*hello*", BUFFER_MESSAGES,
                                       "Olá de um package do mt!\n", true);
    if (buffer)
        snprintf(editor->message, sizeof(editor->message), "Package hello executado");
}

bool mt_package_init(MtAPI *api)
{
    return api->register_command(api->editor, "hello",
                                 "Abre um buffer com uma saudação do package",
                                 COMMAND_FLAG_NONE, hello);
}
