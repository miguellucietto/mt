#ifndef MT_EDITOR_H
#define MT_EDITOR_H
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "dired_controller.h"
#include "document.h"
#include "file_controller.h"
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
    DiredState dired;
    SearchState search;
    FileState files;
    char message[256];
} Editor;
/** Initializes all editor subsystems and optionally opens path. */
bool editor_init(Editor *editor, const char *path);
/** Releases every resource owned by an initialized or partially initialized editor. */
void editor_destroy(Editor *editor);
/** Runs the event-render loop until the editor is asked to stop. */
void editor_run(Editor *editor);
/** Resolves and executes a command while maintaining editor interaction state. */
void editor_execute_named(Editor *editor, const char *name, bool selecting);
/** Adds a command to the editor's unified registry. */
bool editor_register_command(Editor *editor, const char *name, const char *description,
                             unsigned int flags, CommandFunction function);
/** Returns the active buffer, or NULL when no buffer exists. */
Buffer *editor_current_buffer(Editor *editor);
/** Returns the active buffer's document, or NULL when no buffer exists. */
Document *editor_current_document(Editor *editor);
/** Moves the active document cursor and optionally preserves its anchor. */
void editor_set_cursor(Editor *editor, size_t position, bool selecting);
/** Adjusts vertical scrolling so the active cursor remains visible. */
void editor_ensure_cursor_visible(Editor *editor);
/** Maps editor-window coordinates to a byte position in the active document. */
size_t editor_position_from_mouse(const Editor *editor, float x, float y);
#endif
