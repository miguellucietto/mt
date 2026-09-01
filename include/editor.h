#ifndef MT_EDITOR_H
#define MT_EDITOR_H
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "document.h"
#include "keymap.h"
#include "minibuffer.h"
#include "package.h"
#include "search.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#define EDITOR_FONT_SIZE 18.0f
#define EDITOR_GUTTER_WIDTH 58
#define EDITOR_TOP_HEIGHT 40
#define EDITOR_STATUS_HEIGHT 27
#define EDITOR_PADDING 10
typedef struct Editor {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    BufferManager buffers;
    Keymap keymap;
    Config config;
    Minibuffer minibuffer;
    PackageManager packages;
    CommandRegistry commands;
    int width, height, line_height, char_width, scroll_line, wanted_column;
    bool running, dragging;
    bool suppress_text_until_keyup;
    char pending_path[MT_PATH_SIZE];
    SearchState search;
    char message[256];
} Editor;
bool editor_init(Editor *editor, const char *path);
void editor_destroy(Editor *editor);
void editor_run(Editor *editor);
void editor_execute_named(Editor *editor, const char *name, bool selecting);
bool editor_register_command(Editor *editor, const char *name, const char *description,
                             unsigned int flags, CommandFunction function);
Buffer *editor_current_buffer(Editor *editor);
Document *editor_current_document(Editor *editor);
void editor_set_cursor(Editor *editor, size_t position, bool selecting);
void editor_ensure_cursor_visible(Editor *editor);
size_t editor_position_from_mouse(const Editor *editor, float x, float y);
#endif
