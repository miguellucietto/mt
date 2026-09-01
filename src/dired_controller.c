#include "dired_controller.h"

#include "editor.h"
#include "file_controller.h"
#include "text.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool selected_path(Editor *editor, char *path, size_t size)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer || buffer->type != BUFFER_DIRECTORY)
        return false;
    int line = text_line_at(&buffer->document, buffer->document.cursor);
    bool is_directory;
    return buffer_directory_entry(buffer, line, path, size, &is_directory);
}

static bool target_path(Editor *editor, const char *input, char *path, size_t size)
{
    if (!input || !*input)
        return false;
    if (input[0] == '/')
        return snprintf(path, size, "%s", input) < (int)size;
    Buffer *buffer = editor_current_buffer(editor);
    return buffer && buffer->type == BUFFER_DIRECTORY &&
           snprintf(path, size, "%s/%s", buffer->directory, input) < (int)size;
}

static void refresh(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer || buffer->type != BUFFER_DIRECTORY)
        return;
    char directory[MT_PATH_SIZE];
    snprintf(directory, sizeof(directory), "%s", buffer->directory);
    buffer_refresh_directory(buffer, directory, editor->message,
                             sizeof(editor->message));
}

void dired_open_selected(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer || buffer->type != BUFFER_DIRECTORY)
        return;
    int line = text_line_at(&buffer->document, buffer->document.cursor);
    char path[MT_PATH_SIZE];
    bool directory;
    if (!buffer_directory_entry(buffer, line, path, sizeof(path), &directory))
        return;
    if (directory)
        buffer_refresh_directory(buffer, path, editor->message,
                                 sizeof(editor->message));
    else
        file_open(editor, path);
    editor->scroll_line = 0;
}

static void command_dired(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_DIRED, "Dired: ");
}

static void command_open(Editor *editor, bool selecting)
{
    (void)selecting;
    dired_open_selected(editor);
}

static void command_refresh(Editor *editor, bool selecting)
{
    (void)selecting;
    refresh(editor);
}

static void command_create_file(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer && buffer->type == BUFFER_DIRECTORY)
        minibuffer_open(&editor->minibuffer, MINIBUFFER_CREATE_FILE, "Create file: ");
}

static void command_create_directory(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer && buffer->type == BUFFER_DIRECTORY)
        minibuffer_open(&editor->minibuffer, MINIBUFFER_CREATE_DIRECTORY,
                        "Create directory: ");
}

static void command_rename(Editor *editor, bool selecting)
{
    (void)selecting;
    if (selected_path(editor, editor->dired.pending_path,
                      sizeof(editor->dired.pending_path)))
        minibuffer_open(&editor->minibuffer, MINIBUFFER_RENAME, "Rename to: ");
}

static void command_delete(Editor *editor, bool selecting)
{
    (void)selecting;
    if (selected_path(editor, editor->dired.pending_path,
                      sizeof(editor->dired.pending_path)))
        minibuffer_open(&editor->minibuffer, MINIBUFFER_DELETE_CONFIRM,
                        "Delete? type yes: ");
}

bool dired_register_commands(Editor *editor)
{
    static const struct {
        const char *name;
        const char *description;
        unsigned int flags;
        CommandFunction function;
    } commands[] = {
        {"dired", "Abre um diretório", COMMAND_FLAG_OPENS_MINIBUFFER, command_dired},
        {"dired-open", "Abre a entrada selecionada no Dired", 0, command_open},
        {"dired-refresh", "Atualiza a listagem do Dired", 0, command_refresh},
        {"dired-create-file", "Cria um arquivo pelo Dired",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_create_file},
        {"dired-create-directory", "Cria um diretório pelo Dired",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_create_directory},
        {"dired-rename", "Renomeia uma entrada do Dired", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_rename},
        {"dired-delete", "Exclui uma entrada do Dired", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_delete},
    };
    for (size_t i = 0; i < SDL_arraysize(commands); i++)
        if (!editor_register_command(editor, commands[i].name, commands[i].description,
                                     commands[i].flags, commands[i].function))
            return false;
    return true;
}

bool dired_submit(Editor *editor, MinibufferMode mode, const char *value)
{
    char path[MT_PATH_SIZE];
    if (mode == MINIBUFFER_DIRED) {
        Buffer *buffer = buffers_create(&editor->buffers, "*dired*", BUFFER_DIRECTORY);
        if (buffer)
            buffer_refresh_directory(buffer, value, editor->message,
                                     sizeof(editor->message));
    } else if (mode == MINIBUFFER_CREATE_FILE) {
        if (target_path(editor, value, path, sizeof(path))) {
            FILE *file = fopen(path, "wx");
            if (file) {
                fclose(file);
                snprintf(editor->message, sizeof(editor->message), "Criado: %.220s",
                         path);
                refresh(editor);
            } else
                snprintf(editor->message, sizeof(editor->message), "Erro ao criar: %s",
                         strerror(errno));
        }
    } else if (mode == MINIBUFFER_CREATE_DIRECTORY) {
        if (target_path(editor, value, path, sizeof(path))) {
            if (mkdir(path, 0755) == 0) {
                snprintf(editor->message, sizeof(editor->message), "Criado: %.220s",
                         path);
                refresh(editor);
            } else
                snprintf(editor->message, sizeof(editor->message), "Erro ao criar: %s",
                         strerror(errno));
        }
    } else if (mode == MINIBUFFER_RENAME) {
        if (target_path(editor, value, path, sizeof(path))) {
            if (rename(editor->dired.pending_path, path) == 0) {
                snprintf(editor->message, sizeof(editor->message), "Renomeado");
                refresh(editor);
            } else
                snprintf(editor->message, sizeof(editor->message),
                         "Erro ao renomear: %s", strerror(errno));
        }
    } else if (mode == MINIBUFFER_DELETE_CONFIRM) {
        if (strcmp(value, "yes") == 0) {
            struct stat information;
            bool directory = stat(editor->dired.pending_path, &information) == 0 &&
                             S_ISDIR(information.st_mode);
            int result = directory ? rmdir(editor->dired.pending_path)
                                   : remove(editor->dired.pending_path);
            if (result == 0) {
                snprintf(editor->message, sizeof(editor->message), "Excluído");
                refresh(editor);
            } else
                snprintf(editor->message, sizeof(editor->message),
                         "Erro ao excluir: %s", strerror(errno));
        } else
            snprintf(editor->message, sizeof(editor->message), "Exclusão cancelada");
    } else
        return false;
    return true;
}

bool dired_handle_event(Editor *editor, const SDL_Event *event)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer || buffer->type != BUFFER_DIRECTORY ||
        event->type != SDL_EVENT_KEY_DOWN || event->key.key != SDLK_G)
        return false;
    editor_execute_named(editor, "dired-refresh", false);
    return true;
}
