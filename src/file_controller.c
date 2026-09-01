#include "file_controller.h"

#include "editor.h"

#include <stdio.h>
#include <string.h>

void file_open(Editor *editor, const char *path)
{
    if (buffers_file_would_replace_modified(&editor->buffers, path)) {
        snprintf(editor->files.pending_open_path,
                 sizeof(editor->files.pending_open_path), "%s", path);
        minibuffer_open(&editor->minibuffer, MINIBUFFER_REPLACE_BUFFER_CONFIRM,
                        "Buffer modificado. Substituir? digite yes: ");
    } else
        buffers_open_file(&editor->buffers, path, editor->message,
                          sizeof(editor->message));
}

void file_request_quit(Editor *editor)
{
    size_t modified = buffers_modified_count(&editor->buffers);
    if (!modified) {
        editor->running = false;
        return;
    }
    char prompt[64];
    snprintf(prompt, sizeof(prompt),
             "%zu buffer%s modificado%s. Sair? digite yes: ", modified,
             modified == 1 ? "" : "s", modified == 1 ? "" : "s");
    minibuffer_open(&editor->minibuffer, MINIBUFFER_QUIT_CONFIRM, prompt);
}

bool file_submit(Editor *editor, MinibufferMode mode, const char *value)
{
    if (mode == MINIBUFFER_FIND_FILE) {
        file_open(editor, value);
    } else if (mode == MINIBUFFER_REPLACE_BUFFER_CONFIRM) {
        if (strcmp(value, "yes") == 0)
            buffers_open_file_confirmed(&editor->buffers,
                                        editor->files.pending_open_path,
                                        editor->message, sizeof(editor->message));
        else
            snprintf(editor->message, sizeof(editor->message),
                     "Substituição de buffer cancelada");
    } else if (mode == MINIBUFFER_QUIT_CONFIRM) {
        if (strcmp(value, "yes") == 0)
            editor->running = false;
        else
            snprintf(editor->message, sizeof(editor->message), "Saída cancelada");
    } else
        return false;
    return true;
}

/* Saves the active document and reports the persistence result to the editor. */
static void command_save(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only)
        document_save(&buffer->document, editor->message, sizeof(editor->message));
}

/* Opens the path prompt used by the find-file command. */
static void command_find_file(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_FIND_FILE, "Find file: ");
}

/* Routes quit command invocation through modified-buffer protection. */
static void command_quit(Editor *editor, bool selecting)
{
    (void)selecting;
    file_request_quit(editor);
}

bool file_register_commands(Editor *editor)
{
    return command_registry_register(&editor->commands, "save", "Salva o buffer atual",
                                     COMMAND_FLAG_NONE, command_save) &&
           command_registry_register(&editor->commands, "find-file", "Abre um arquivo",
                                     COMMAND_FLAG_OPENS_MINIBUFFER,
                                     command_find_file) &&
           command_registry_register(&editor->commands, "quit", "Encerra o editor",
                                     COMMAND_FLAG_NONE, command_quit);
}
