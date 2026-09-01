#include "shell_controller.h"

#include "editor.h"
#include "process.h"

#include <stdio.h>

static void command_shell(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_SHELL, "Shell command: ");
}

bool shell_register_commands(Editor *editor)
{
    return editor_register_command(editor, "cmd", "Executa um comando do shell",
                                   COMMAND_FLAG_OPENS_MINIBUFFER, command_shell);
}

bool shell_submit(Editor *editor, MinibufferMode mode, const char *value)
{
    if (mode != MINIBUFFER_SHELL)
        return false;
    ProcessResult result;
    char error[256];
    if (!process_run_shell(value, &result, error, sizeof(error))) {
        snprintf(editor->message, sizeof(editor->message), "%s", error);
        return true;
    }

    Buffer *buffer =
        buffers_open_text(&editor->buffers, "*cmd*", BUFFER_SHELL, "$ ", false);
    bool complete = false;
    if (buffer) {
        buffer->document.cursor = buffer->document.anchor = buffer->document.length;
        complete = document_insert(&buffer->document, value) &&
                   document_insert(&buffer->document, "\n\n") &&
                   document_insert(&buffer->document, result.output);
        buffer->document.dirty = false;
        buffer->read_only = true;
    }
    editor->scroll_line = 0;
    if (complete)
        snprintf(editor->message, sizeof(editor->message), "Comando finalizado (%d)",
                 result.status);
    else
        snprintf(editor->message, sizeof(editor->message),
                 "Unable to display complete command output");
    process_result_destroy(&result);
    return true;
}
