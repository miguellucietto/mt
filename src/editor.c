#include "editor.h"
#include "editing.h"
#include "renderer.h"
#include "shell_controller.h"
#include "text.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Registers coordinator-owned commands after each feature controller. */
static bool register_native_commands(Editor *editor);

/* Locates the configured or first available platform monospace font. */
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
    settings_init_defaults(&editor->settings);
    editor->wanted_column = -1;
    editor->running = true;
    packages_init(&editor->packages);
    command_registry_init(&editor->commands);
    if (!register_native_commands(editor))
        return false;
    if (!buffers_init(&editor->buffers))
        return false;
    if (!config_paths_discover(&editor->config_paths, editor->message,
                               sizeof(editor->message)) ||
        !config_paths_prepare(&editor->config_paths, editor->message,
                              sizeof(editor->message)))
        goto error;
    if (!settings_ensure_file(editor->config_paths.settings_path, editor->message,
                              sizeof(editor->message)))
        goto error;
    if (!settings_load_file(&editor->settings, editor->config_paths.settings_path,
                            editor->message, sizeof(editor->message)))
        goto error;
    editor->width = editor->settings.window_width;
    editor->height = editor->settings.window_height;
    keymap_init_default(&editor->keymap);
    keymap_load(&editor->keymap, editor->config_paths.keymap_path, editor->message,
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
    if (!font || !(editor->font = TTF_OpenFont(font, editor->settings.font_size)))
        goto error;
    if (!TTF_GetStringSize(editor->font, "M", 1, &editor->char_width,
                           &editor->line_height))
        goto error;
    editor->line_height += editor->settings.line_spacing;
    SDL_StartTextInput(editor->window);
    packages_load_directory(&editor->packages, editor,
                            editor->config_paths.packages_path, editor->message,
                            sizeof(editor->message));
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
    int visible = (editor->height - editor->settings.top_height -
                   editor->settings.status_height) /
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
    int line = editor->scroll_line +
               (int)(y - editor->settings.top_height) / editor->line_height;
    int column = (int)(x - editor->settings.gutter_width - editor->settings.padding +
                       editor->char_width / 2) /
                 editor->char_width;
    if (line < 0)
        line = 0;
    if (column < 0)
        column = 0;
    return text_position_at(&buffer->document, line, column);
}

/* Builds a read-only buffer describing the current unified command registry. */
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

/* Inserts a newline or delegates Enter to the active directory controller. */
static void command_newline(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer->type == BUFFER_DIRECTORY)
        dired_open_selected(editor);
    else if (!buffer->read_only)
        document_insert(&buffer->document, "\n");
}

/* Opens the named-command prompt used by M-x. */
static void command_execute_command(Editor *editor, bool selecting)
{
    (void)selecting;
    minibuffer_open(&editor->minibuffer, MINIBUFFER_COMMAND, "M-x ");
}

/* Activates the next buffer and resets its initial vertical viewport. */
static void command_next_buffer(Editor *editor, bool selecting)
{
    (void)selecting;
    buffers_next(&editor->buffers);
    editor->scroll_line = 0;
}

/* Presents all commands through the registry-generated command buffer. */
static void command_list_commands(Editor *editor, bool selecting)
{
    (void)selecting;
    show_commands(editor);
}

typedef struct {
    const char *name;
    const char *description;
    unsigned int flags;
    CommandFunction function;
} NativeCommand;

/* Composes controller registrations with the small coordinator command set. */
static bool register_native_commands(Editor *editor)
{
    if (!editing_register_commands(editor))
        return false;
    if (!search_register_commands(editor))
        return false;
    if (!file_register_commands(editor))
        return false;
    if (!dired_register_commands(editor))
        return false;
    if (!shell_register_commands(editor))
        return false;
    static const NativeCommand commands[] = {
        {"newline", "Insere uma nova linha ou abre a entrada do Dired", 0,
         command_newline},
        {"execute-command", "Executa um comando pelo nome",
         COMMAND_FLAG_OPENS_MINIBUFFER, command_execute_command},
        {"next-buffer", "Alterna para o próximo buffer", 0, command_next_buffer},
        {"list-commands", "Lista todos os comandos registrados", 0,
         command_list_commands},
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

/* Normalizes prompt input and dispatches it to the owning controller. */
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
    else if (!shell_submit(editor, mode, value) && !dired_submit(editor, mode, value) &&
             !file_submit(editor, mode, value))
        search_submit(editor, mode, value);
    editor->scroll_line = 0;
}

/* Routes input to an active minibuffer session and reports event consumption. */
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
    if (search_handle_confirmation(editor, event))
        return true;
    if (event->type == SDL_EVENT_TEXT_INPUT)
        minibuffer_insert(&editor->minibuffer, event->text.text);
    else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            if (editor->minibuffer.mode == MINIBUFFER_ISEARCH) {
                search_cancel(editor);
            }
            minibuffer_close(&editor->minibuffer);
        } else if (event->key.key == SDLK_BACKSPACE)
            minibuffer_backspace(&editor->minibuffer);
        else if (editor->minibuffer.mode == MINIBUFFER_ISEARCH &&
                 event->key.key == SDLK_F && (event->key.mod & SDL_KMOD_CTRL)) {
            search_next(editor);
        } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER)
            submit_minibuffer(editor);
    }
    if (editor->minibuffer.mode == MINIBUFFER_ISEARCH &&
        (event->type == SDL_EVENT_TEXT_INPUT ||
         (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_BACKSPACE)))
        search_update(editor);
    return event->type == SDL_EVENT_TEXT_INPUT || event->type == SDL_EVENT_KEY_DOWN;
}

/* Coordinates one SDL event across lifecycle, controllers, keymaps, and input. */
static void handle_event(Editor *editor, const SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        file_request_quit(editor);
        return;
    }
    if (handle_minibuffer_event(editor, event))
        return;
    if (dired_handle_event(editor, event))
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
        const char *command = keymap_lookup(&editor->keymap, &event->key);
        if (command) {
            const CommandSpec *spec = command_registry_find(&editor->commands, command);
            if (spec && (spec->flags & COMMAND_FLAG_OPENS_MINIBUFFER))
                editor->suppress_text_until_keyup = true;
            editor_execute_named(editor, command,
                                 (event->key.mod & SDL_KMOD_SHIFT) != 0);
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
