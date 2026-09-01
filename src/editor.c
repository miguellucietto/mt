#include "editor.h"
#include "editing.h"
#include "renderer.h"
#include "text.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool register_native_commands(Editor *editor);

static const char *find_font(void)
{
    const char *override = getenv("MT_FONT");
    if (override && *override)
        return override;
    static const char *fonts[] = {"/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
                                  "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                                  "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                                  "/System/Library/Fonts/Menlo.ttc",
                                  "C:\\Windows\\Fonts\\consola.ttf"};
    for (size_t i = 0; i < SDL_arraysize(fonts); i++) {
        FILE *file = fopen(fonts[i], "rb");
        if (file) {
            fclose(file);
            return fonts[i];
        }
    }
    return NULL;
}

Buffer *editor_current_buffer(Editor *editor)
{
    return buffers_current(&editor->buffers);
}

Document *editor_current_document(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    return buffer ? &buffer->document : NULL;
}

bool editor_register_command(Editor *editor, const char *name, const char *description,
                             unsigned int flags, CommandFunction function)
{
    return command_registry_register(&editor->commands, name, description, flags,
                                     function);
}

bool editor_init(Editor *editor, const char *path)
{
    memset(editor, 0, sizeof(*editor));
    editor->width = 1000;
    editor->height = 700;
    editor->wanted_column = -1;
    editor->running = true;
    packages_init(&editor->packages);
    command_registry_init(&editor->commands);
    if (!register_native_commands(editor))
        return false;
    if (!buffers_init(&editor->buffers))
        return false;
    config_init(&editor->config, editor->message, sizeof(editor->message));
    keymap_init_default(&editor->keymap);
    keymap_load(&editor->keymap, editor->config.keymap_path, editor->message,
                sizeof(editor->message));
    if (path)
        buffers_open_file(&editor->buffers, path, editor->message,
                          sizeof(editor->message));
    if (!SDL_Init(SDL_INIT_VIDEO) || !TTF_Init())
        goto error;
    if (!SDL_CreateWindowAndRenderer("mt", editor->width, editor->height,
                                     SDL_WINDOW_RESIZABLE, &editor->window,
                                     &editor->renderer))
        goto error;
    const char *font = find_font();
    if (!font || !(editor->font = TTF_OpenFont(font, EDITOR_FONT_SIZE)))
        goto error;
    if (!TTF_GetStringSize(editor->font, "M", 1, &editor->char_width,
                           &editor->line_height))
        goto error;
    editor->line_height += 4;
    SDL_StartTextInput(editor->window);
    packages_load_directory(&editor->packages, editor, editor->config.packages_path,
                            editor->message, sizeof(editor->message));
    return true;
error:
    editor_destroy(editor);
    return false;
}

void editor_destroy(Editor *editor)
{
    packages_destroy(&editor->packages);
    if (editor->window)
        SDL_StopTextInput(editor->window);
    if (editor->font)
        TTF_CloseFont(editor->font);
    if (editor->renderer)
        SDL_DestroyRenderer(editor->renderer);
    if (editor->window)
        SDL_DestroyWindow(editor->window);
    TTF_Quit();
    SDL_Quit();
    buffers_destroy(&editor->buffers);
}

void editor_ensure_cursor_visible(Editor *editor)
{
    Document *document = editor_current_document(editor);
    if (!document || !editor->line_height)
        return;
    int line = text_line_at(document, document->cursor);
    int visible = (editor->height - EDITOR_TOP_HEIGHT - EDITOR_STATUS_HEIGHT) /
                  editor->line_height;
    if (line < editor->scroll_line)
        editor->scroll_line = line;
    if (line >= editor->scroll_line + visible)
        editor->scroll_line = line - visible + 1;
    if (editor->scroll_line < 0)
        editor->scroll_line = 0;
}

void editor_set_cursor(Editor *editor, size_t position, bool selecting)
{
    Document *document = editor_current_document(editor);
    if (!document)
        return;
    document->cursor = position;
    if (!selecting)
        document->anchor = position;
    editor_ensure_cursor_visible(editor);
}

size_t editor_position_from_mouse(const Editor *editor, float x, float y)
{
    const Buffer *buffer = buffers_current_const(&editor->buffers);
    if (!buffer)
        return 0;
    int line = editor->scroll_line + (int)(y - EDITOR_TOP_HEIGHT) / editor->line_height;
    int column =
        (int)(x - EDITOR_GUTTER_WIDTH - EDITOR_PADDING + editor->char_width / 2) /
        editor->char_width;
    if (line < 0)
        line = 0;
    if (column < 0)
        column = 0;
    return text_position_at(&buffer->document, line, column);
}

static bool find_text(Editor *editor, size_t start, bool wrap)
{
    Document *document = editor_current_document(editor);
    if (!editor->search_text[0]) {
        document->cursor = document->anchor = editor->search_origin;
        return false;
    }
    const char *match = strstr(document->text + start, editor->search_text);
    if (!match && wrap && start > 0) {
        match = strstr(document->text, editor->search_text);
        if (match && (size_t)(match - document->text) >= start)
            match = NULL;
    }
    if (!match)
        return false;
    document->anchor = (size_t)(match - document->text);
    document->cursor = document->anchor + strlen(editor->search_text);
    editor_ensure_cursor_visible(editor);
    return true;
}

static void update_isearch(Editor *editor)
{
    snprintf(editor->search_text, sizeof(editor->search_text), "%s",
             editor->minibuffer.input);
    if (!find_text(editor, editor->search_origin, true) && editor->search_text[0])
        snprintf(editor->message, sizeof(editor->message), "Sem resultado: %.180s",
                 editor->search_text);
}

static bool query_next(Editor *editor)
{
    Document *document = editor_current_document(editor);
    size_t start = document_selection_end(document);
    if (!find_text(editor, start, false)) {
        minibuffer_close(&editor->minibuffer);
        snprintf(editor->message, sizeof(editor->message), "Query replace concluído");
        return false;
    }
    minibuffer_open(&editor->minibuffer, MINIBUFFER_QUERY_CONFIRM,
                    "Replace? y/n/!/q: ");
    return true;
}

static void replace_current_match(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only)
        document_insert(&buffer->document, editor->replace_text);
}

static void execute_shell(Editor *editor, const char *command)
{
    char shell_command[2048];
    snprintf(shell_command, sizeof(shell_command), "%s 2>&1", command);
    FILE *pipe = popen(shell_command, "r");
    if (!pipe) {
        snprintf(editor->message, sizeof(editor->message),
                 "Não foi possível executar o comando");
        return;
    }
    size_t capacity = 4096, length = 0;
    char *output = malloc(capacity);
    if (!output) {
        pclose(pipe);
        return;
    }
    int character;
    while ((character = fgetc(pipe)) != EOF) {
        if (length + 2 > capacity) {
            capacity *= 2;
            char *larger = realloc(output, capacity);
            if (!larger)
                break;
            output = larger;
        }
        output[length++] = (char)character;
    }
    int status = pclose(pipe);
    output[length] = '\0';
    char contents[256];
    snprintf(contents, sizeof(contents), "$ %s\n\n", command);
    Buffer *buffer =
        buffers_open_text(&editor->buffers, "*cmd*", BUFFER_SHELL, contents, false);
    if (buffer) {
        buffer->document.cursor = buffer->document.anchor = buffer->document.length;
        document_insert(&buffer->document, output);
        buffer->document.dirty = false;
        buffer->read_only = true;
    }
    free(output);
    editor->scroll_line = 0;
    snprintf(editor->message, sizeof(editor->message), "Comando finalizado (%d)",
             status);
}

static void open_dired_entry(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    int line = text_line_at(&buffer->document, buffer->document.cursor);
    char path[MT_PATH_SIZE];
    bool directory;
    if (!buffer_directory_entry(buffer, line, path, sizeof(path), &directory))
        return;
    if (directory)
        buffer_refresh_directory(buffer, path, editor->message,
                                 sizeof(editor->message));
    else if (buffers_file_would_replace_modified(&editor->buffers, path)) {
        snprintf(editor->pending_path, sizeof(editor->pending_path), "%s", path);
        minibuffer_open(&editor->minibuffer, MINIBUFFER_REPLACE_BUFFER_CONFIRM,
                        "Buffer modificado. Substituir? digite yes: ");
    } else
        buffers_open_file(&editor->buffers, path, editor->message,
                          sizeof(editor->message));
    editor->scroll_line = 0;
}

static bool selected_dired_path(Editor *editor, char *path, size_t size)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer->type != BUFFER_DIRECTORY)
        return false;
    int line = text_line_at(&buffer->document, buffer->document.cursor);
    bool is_directory;
    return buffer_directory_entry(buffer, line, path, size, &is_directory);
}

static bool dired_target_path(Editor *editor, const char *input, char *path,
                              size_t size)
{
    if (!input || !*input)
        return false;
    if (input[0] == '/')
        return snprintf(path, size, "%s", input) < (int)size;
    Buffer *buffer = editor_current_buffer(editor);
    return buffer->type == BUFFER_DIRECTORY &&
           snprintf(path, size, "%s/%s", buffer->directory, input) < (int)size;
}

static void refresh_current_dired(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer->type == BUFFER_DIRECTORY) {
        char directory[MT_PATH_SIZE];
        snprintf(directory, sizeof(directory), "%s", buffer->directory);
        buffer_refresh_directory(buffer, directory, editor->message,
                                 sizeof(editor->message));
    }
}

static void show_commands(Editor *editor)
{
    char contents[16384];
    size_t used = 0;
    used += (size_t)snprintf(contents + used, sizeof(contents) - used,
                             "Comandos do mt\n============\n\n");
    for (size_t i = 0; i < editor->commands.count && used < sizeof(contents); i++) {
        const CommandSpec *command = command_registry_at(&editor->commands, i);
        used += (size_t)snprintf(contents + used, sizeof(contents) - used,
                                 "  %-28s %s\n", command->name, command->description);
    }
    buffers_open_text(&editor->buffers, "*commands*", BUFFER_MESSAGES, contents, true);
    editor->scroll_line = 0;
}

static void request_quit(Editor *editor)
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

static void command_newline(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer->type == BUFFER_DIRECTORY)
        open_dired_entry(editor);
    else if (!buffer->read_only)
        document_insert(&buffer->document, "\n");
}

static void command_isearch(Editor *editor, bool selecting)
{
    (void)selecting;
    editor->search_origin = editor_current_document(editor)->cursor;
    editor->search_text[0] = '\0';
    minibuffer_open(&editor->minibuffer, MINIBUFFER_ISEARCH, "I-search: ");
}

static void command_query_replace(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only) {
        editor->search_origin = buffer->document.cursor;
        minibuffer_open(&editor->minibuffer, MINIBUFFER_QUERY_FIND, "Query replace: ");
    }
}

static void command_execute_command(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_COMMAND, "M-x ");
}

static void command_shell(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_SHELL, "Shell command: ");
}

static void command_find_file(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_FIND_FILE, "Find file: ");
}

static void command_dired(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_DIRED, "Dired: ");
}

static void command_next_buffer(Editor *editor, bool selecting)
{
    (void)selecting;
    buffers_next(&editor->buffers);
    editor->scroll_line = 0;
}

static void command_dired_open(Editor *editor, bool selecting)
{
    (void)selecting;
    open_dired_entry(editor);
}

static void command_dired_refresh(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer->type == BUFFER_DIRECTORY)
        buffer_refresh_directory(buffer, buffer->directory, editor->message,
                                 sizeof(editor->message));
}

static void command_dired_create_file(Editor *editor, bool selecting)
{
    (void)selecting;
    if (editor_current_buffer(editor)->type == BUFFER_DIRECTORY)
        minibuffer_open(&editor->minibuffer, MINIBUFFER_CREATE_FILE, "Create file: ");
}

static void command_dired_create_directory(Editor *editor, bool selecting)
{
    (void)selecting;
    if (editor_current_buffer(editor)->type == BUFFER_DIRECTORY)
        minibuffer_open(&editor->minibuffer, MINIBUFFER_CREATE_DIRECTORY,
                        "Create directory: ");
}

static void command_dired_rename(Editor *editor, bool selecting)
{
    (void)selecting;
    if (selected_dired_path(editor, editor->pending_path, sizeof(editor->pending_path)))
        minibuffer_open(&editor->minibuffer, MINIBUFFER_RENAME, "Rename to: ");
}

static void command_dired_delete(Editor *editor, bool selecting)
{
    (void)selecting;
    if (selected_dired_path(editor, editor->pending_path, sizeof(editor->pending_path)))
        minibuffer_open(&editor->minibuffer, MINIBUFFER_DELETE_CONFIRM,
                        "Delete? type yes: ");
}

static void command_list_commands(Editor *editor, bool selecting)
{
    (void)selecting;
    show_commands(editor);
}

static void command_quit(Editor *editor, bool selecting)
{
    (void)selecting;
    request_quit(editor);
}

typedef struct {
    const char *name;
    const char *description;
    unsigned int flags;
    CommandFunction function;
} NativeCommand;

static bool register_native_commands(Editor *editor)
{
    if (!editing_register_commands(editor))
        return false;
    static const NativeCommand commands[] = {
        {"newline", "Insere uma nova linha ou abre a entrada do Dired", 0,
         command_newline},
        {"isearch", "Inicia busca incremental", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_isearch},
        {"query-replace", "Substitui ocorrências com confirmação",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_query_replace},
        {"execute-command", "Executa um comando pelo nome",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_execute_command},
        {"cmd", "Executa um comando do shell", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_shell},
        {"find-file", "Abre um arquivo", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_find_file},
        {"dired", "Abre um diretório", COMMAND_FLAG_OPENS_MINIBUFFER, command_dired},
        {"next-buffer", "Alterna para o próximo buffer", 0, command_next_buffer},
        {"dired-open", "Abre a entrada selecionada no Dired", 0, command_dired_open},
        {"dired-refresh", "Atualiza a listagem do Dired", 0, command_dired_refresh},
        {"dired-create-file", "Cria um arquivo pelo Dired",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_dired_create_file},
        {"dired-create-directory", "Cria um diretório pelo Dired",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_dired_create_directory},
        {"dired-rename", "Renomeia uma entrada do Dired", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_dired_rename},
        {"dired-delete", "Exclui uma entrada do Dired", COMMAND_FLAG_OPENS_MINIBUFFER,
         command_dired_delete},
        {"list-commands", "Lista todos os comandos registrados", 0,
         command_list_commands},
        {"quit", "Encerra o editor", 0, command_quit},
    };
    for (size_t i = 0; i < SDL_arraysize(commands); i++)
        if (!command_registry_register(&editor->commands, commands[i].name,
                                       commands[i].description, commands[i].flags,
                                       commands[i].function))
            return false;
    return true;
}

void editor_execute_named(Editor *editor, const char *name, bool selecting)
{
    if (!name || !*name) {
        show_commands(editor);
        return;
    }
    const CommandSpec *command = command_registry_find(&editor->commands, name);
    if (!command) {
        snprintf(editor->message, sizeof(editor->message), "Comando desconhecido: %s",
                 name);
        return;
    }
    Document *document = editor_current_document(editor);
    if (document)
        document_break_undo_group(document);
    if (!command_registry_execute(&editor->commands, name, editor, selecting))
        return;
    if (!(command->flags & COMMAND_FLAG_KEEP_COLUMN))
        editor->wanted_column = -1;
    editor_ensure_cursor_visible(editor);
}

static void submit_minibuffer(Editor *editor)
{
    MinibufferMode mode = editor->minibuffer.mode;
    char input[sizeof(editor->minibuffer.input)];
    snprintf(input, sizeof(input), "%s", editor->minibuffer.input);
    char *value = input;
    while (*value == ' ' || *value == '\t')
        value++;
    char *end = value + strlen(value);
    while (end > value && (end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
    minibuffer_close(&editor->minibuffer);
    if (mode == MINIBUFFER_COMMAND)
        editor_execute_named(editor, value, false);
    else if (mode == MINIBUFFER_SHELL)
        execute_shell(editor, value);
    else if (mode == MINIBUFFER_FIND_FILE) {
        if (buffers_file_would_replace_modified(&editor->buffers, value)) {
            snprintf(editor->pending_path, sizeof(editor->pending_path), "%s", value);
            minibuffer_open(&editor->minibuffer, MINIBUFFER_REPLACE_BUFFER_CONFIRM,
                            "Buffer modificado. Substituir? digite yes: ");
        } else
            buffers_open_file(&editor->buffers, value, editor->message,
                              sizeof(editor->message));
    } else if (mode == MINIBUFFER_DIRED) {
        Buffer *buffer = buffers_create(&editor->buffers, "*dired*", BUFFER_DIRECTORY);
        if (buffer)
            buffer_refresh_directory(buffer, value, editor->message,
                                     sizeof(editor->message));
    } else if (mode == MINIBUFFER_CREATE_FILE) {
        char path[MT_PATH_SIZE];
        if (dired_target_path(editor, value, path, sizeof(path))) {
            FILE *file = fopen(path, "wx");
            if (file) {
                fclose(file);
                snprintf(editor->message, sizeof(editor->message), "Criado: %.220s",
                         path);
                refresh_current_dired(editor);
            } else
                snprintf(editor->message, sizeof(editor->message), "Erro ao criar: %s",
                         strerror(errno));
        }
    } else if (mode == MINIBUFFER_CREATE_DIRECTORY) {
        char path[MT_PATH_SIZE];
        if (dired_target_path(editor, value, path, sizeof(path))) {
            if (mkdir(path, 0755) == 0) {
                snprintf(editor->message, sizeof(editor->message), "Criado: %.220s",
                         path);
                refresh_current_dired(editor);
            } else
                snprintf(editor->message, sizeof(editor->message), "Erro ao criar: %s",
                         strerror(errno));
        }
    } else if (mode == MINIBUFFER_RENAME) {
        char target[MT_PATH_SIZE];
        if (dired_target_path(editor, value, target, sizeof(target))) {
            if (rename(editor->pending_path, target) == 0) {
                snprintf(editor->message, sizeof(editor->message), "Renomeado");
                refresh_current_dired(editor);
            } else
                snprintf(editor->message, sizeof(editor->message),
                         "Erro ao renomear: %s", strerror(errno));
        }
    } else if (mode == MINIBUFFER_DELETE_CONFIRM && strcmp(value, "yes") == 0) {
        struct stat information;
        bool directory = stat(editor->pending_path, &information) == 0 &&
                         S_ISDIR(information.st_mode);
        int result =
            directory ? rmdir(editor->pending_path) : remove(editor->pending_path);
        if (result == 0) {
            snprintf(editor->message, sizeof(editor->message), "Excluído");
            refresh_current_dired(editor);
        } else
            snprintf(editor->message, sizeof(editor->message), "Erro ao excluir: %s",
                     strerror(errno));
    } else if (mode == MINIBUFFER_DELETE_CONFIRM) {
        snprintf(editor->message, sizeof(editor->message), "Exclusão cancelada");
    } else if (mode == MINIBUFFER_REPLACE_BUFFER_CONFIRM && strcmp(value, "yes") == 0) {
        buffers_open_file_confirmed(&editor->buffers, editor->pending_path,
                                    editor->message, sizeof(editor->message));
    } else if (mode == MINIBUFFER_REPLACE_BUFFER_CONFIRM) {
        snprintf(editor->message, sizeof(editor->message),
                 "Substituição de buffer cancelada");
    } else if (mode == MINIBUFFER_QUIT_CONFIRM && strcmp(value, "yes") == 0) {
        editor->running = false;
    } else if (mode == MINIBUFFER_QUIT_CONFIRM) {
        snprintf(editor->message, sizeof(editor->message), "Saída cancelada");
    } else if (mode == MINIBUFFER_ISEARCH) {
        snprintf(editor->search_text, sizeof(editor->search_text), "%s", value);
    } else if (mode == MINIBUFFER_QUERY_FIND) {
        snprintf(editor->search_text, sizeof(editor->search_text), "%s", value);
        minibuffer_open(&editor->minibuffer, MINIBUFFER_QUERY_REPLACE,
                        "Replace with: ");
    } else if (mode == MINIBUFFER_QUERY_REPLACE) {
        snprintf(editor->replace_text, sizeof(editor->replace_text), "%s", value);
        Document *document = editor_current_document(editor);
        document->cursor = document->anchor = editor->search_origin;
        query_next(editor);
    }
    editor->scroll_line = 0;
}

static bool handle_minibuffer_event(Editor *editor, const SDL_Event *event)
{
    if (editor->minibuffer.mode == MINIBUFFER_INACTIVE)
        return false;
    if (event->type == SDL_EVENT_KEY_UP && editor->suppress_text_until_keyup) {
        editor->suppress_text_until_keyup = false;
        return true;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT && editor->suppress_text_until_keyup)
        return true;
    if (editor->minibuffer.mode == MINIBUFFER_QUERY_CONFIRM) {
        if (event->type != SDL_EVENT_KEY_DOWN)
            return true;
        SDL_Keycode key = event->key.key;
        if (key == SDLK_Y) {
            replace_current_match(editor);
            query_next(editor);
        } else if (key == SDLK_N) {
            Document *document = editor_current_document(editor);
            document->cursor = document->anchor = document_selection_end(document);
            query_next(editor);
        } else if (key == SDLK_EXCLAIM ||
                   (key == SDLK_1 && (event->key.mod & SDL_KMOD_SHIFT))) {
            do {
                replace_current_match(editor);
            } while (query_next(editor));
        } else if (key == SDLK_Q || key == SDLK_ESCAPE) {
            minibuffer_close(&editor->minibuffer);
            snprintf(editor->message, sizeof(editor->message),
                     "Query replace encerrado");
        }
        return true;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT)
        minibuffer_insert(&editor->minibuffer, event->text.text);
    else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            if (editor->minibuffer.mode == MINIBUFFER_ISEARCH) {
                Document *document = editor_current_document(editor);
                document->cursor = document->anchor = editor->search_origin;
            }
            minibuffer_close(&editor->minibuffer);
        } else if (event->key.key == SDLK_BACKSPACE)
            minibuffer_backspace(&editor->minibuffer);
        else if (editor->minibuffer.mode == MINIBUFFER_ISEARCH &&
                 event->key.key == SDLK_F && (event->key.mod & SDL_KMOD_CTRL)) {
            Document *document = editor_current_document(editor);
            find_text(editor, document_selection_end(document), true);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER)
            submit_minibuffer(editor);
    }
    if (editor->minibuffer.mode == MINIBUFFER_ISEARCH &&
        (event->type == SDL_EVENT_TEXT_INPUT ||
         (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_BACKSPACE)))
        update_isearch(editor);
    return event->type == SDL_EVENT_TEXT_INPUT || event->type == SDL_EVENT_KEY_DOWN;
}

static void handle_event(Editor *editor, const SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        request_quit(editor);
        return;
    }
    if (handle_minibuffer_event(editor, event))
        return;
    Buffer *buffer = editor_current_buffer(editor);
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        editor->width = event->window.data1;
        editor->height = event->window.data2;
        editor_ensure_cursor_visible(editor);
    } else if (event->type == SDL_EVENT_TEXT_INPUT) {
        if (!buffer->read_only)
            document_insert_typed(&buffer->document, event->text.text);
        editor_ensure_cursor_visible(editor);
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (buffer->type == BUFFER_DIRECTORY && event->key.key == SDLK_G)
            editor_execute_named(editor, "dired-refresh", false);
        else {
            const char *command = keymap_lookup(&editor->keymap, &event->key);
            if (command) {
                const CommandSpec *spec =
                    command_registry_find(&editor->commands, command);
                if (spec && (spec->flags & COMMAND_FLAG_OPENS_MINIBUFFER))
                    editor->suppress_text_until_keyup = true;
                editor_execute_named(editor, command,
                                     (event->key.mod & SDL_KMOD_SHIFT) != 0);
            }
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        editor->scroll_line -= (int)(event->wheel.y * 3);
        if (editor->scroll_line < 0)
            editor->scroll_line = 0;
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
               event->button.button == SDL_BUTTON_LEFT) {
        document_break_undo_group(&buffer->document);
        editor_set_cursor(
            editor,
            editor_position_from_mouse(editor, event->button.x, event->button.y),
            (SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
        editor->dragging = true;
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
               event->button.button == SDL_BUTTON_LEFT)
        editor->dragging = false;
    else if (event->type == SDL_EVENT_MOUSE_MOTION && editor->dragging)
        editor_set_cursor(
            editor,
            editor_position_from_mouse(editor, event->motion.x, event->motion.y), true);
}

void editor_run(Editor *editor)
{
    while (editor->running) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            handle_event(editor, &event);
        editor_render(editor);
        SDL_Delay(16);
    }
}
