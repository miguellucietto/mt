#include "renderer.h"
#include "highlight.h"
#include "text.h"

#include <stdio.h>
#include <string.h>

/* Fills one rectangle using the editor's active SDL renderer. */
static void fill(Editor *editor, SDL_FRect rectangle, SDL_Color color)
{
    SDL_SetRenderDrawColor(editor->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(editor->renderer, &rectangle);
}

/* Renders an explicit UTF-8 byte range and returns its measured width if requested. */
static void draw_text(Editor *editor, const char *text, size_t length, float x, float y,
                      SDL_Color color)
{
    if (!length)
        return;
    SDL_Surface *surface = TTF_RenderText_Blended(editor->font, text, length, color);
    if (!surface)
        return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(editor->renderer, surface);
    SDL_FRect target = {x, y, (float)surface->w, (float)surface->h};
    SDL_DestroySurface(surface);
    if (texture) {
        SDL_RenderTexture(editor->renderer, texture, NULL, &target);
        SDL_DestroyTexture(texture);
    }
}

/* Reports whether the active buffer path selects the built-in C highlighter. */
static bool is_c_buffer(const Buffer *buffer)
{
    const char *path = buffer->document.path;
    if (!path)
        return false;
    const char *extension = strrchr(path, '.');
    return extension && (strcmp(extension, ".c") == 0 || strcmp(extension, ".h") == 0);
}

/* Draws one line by mapping lexer spans to colored text runs. */
static void draw_highlighted_line(Editor *editor, const char *text, size_t length,
                                  float x, float y)
{
    HighlightSpan spans[128];
    size_t count = highlight_c_line(text, length, spans, SDL_arraysize(spans));
    size_t position = 0;
    for (size_t i = 0; i < count; i++) {
        if (spans[i].start > position)
            draw_text(editor, text + position, spans[i].start - position,
                      x + position * editor->char_width, y,
                      highlight_color(HIGHLIGHT_NORMAL));
        draw_text(editor, text + spans[i].start, spans[i].length,
                  x + spans[i].start * editor->char_width, y,
                  highlight_color(spans[i].kind));
        position = spans[i].start + spans[i].length;
    }
    if (position < length)
        draw_text(editor, text + position, length - position,
                  x + position * editor->char_width, y,
                  highlight_color(HIGHLIGHT_NORMAL));
}

void editor_render(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    Document *document = &buffer->document;
    const SDL_Color background = {30, 32, 40, 255};
    const SDL_Color panel = {23, 25, 31, 255};
    const SDL_Color foreground = {224, 226, 235, 255};
    const SDL_Color muted = {125, 132, 150, 255};
    const SDL_Color accent = {105, 155, 255, 255};
    const SDL_Color selection = {55, 78, 125, 255};
    const Settings *settings = &editor->settings;
    SDL_SetRenderDrawColor(editor->renderer, background.r, background.g, background.b,
                           255);
    SDL_RenderClear(editor->renderer);
    fill(editor, (SDL_FRect){0, 0, editor->width, settings->top_height}, panel);
    fill(editor,
         (SDL_FRect){0, editor->height - settings->status_height, editor->width,
                     settings->status_height},
         panel);
    fill(editor,
         (SDL_FRect){0, settings->top_height, settings->gutter_width,
                     editor->height - settings->top_height - settings->status_height},
         panel);

    char title[512];
    snprintf(title, sizeof(title), "mt  [%zu/%zu]  %s%s", editor->buffers.active + 1,
             editor->buffers.count, buffer->name, document->dirty ? "  *" : "");
    draw_text(editor, title, strlen(title), settings->padding, 9, foreground);

    int first = editor->scroll_line;
    int last = first +
               (editor->height - settings->top_height - settings->status_height) /
                   editor->line_height +
               1;
    int line = 0;
    size_t start = 0;
    size_t selection_start = document_selection_start(document);
    size_t selection_end = document_selection_end(document);
    while (start <= document->length) {
        size_t end = text_line_end(document, start);
        if (line >= first && line <= last) {
            float y =
                (float)(settings->top_height + (line - first) * editor->line_height);
            char number[16];
            snprintf(number, sizeof(number), "%d", line + 1);
            draw_text(editor, number, strlen(number),
                      settings->gutter_width - settings->padding -
                          strlen(number) * editor->char_width,
                      y, muted);
            if (selection_start != selection_end && selection_start <= end &&
                selection_end >= start) {
                size_t a = selection_start > start ? selection_start : start;
                size_t b = selection_end < end ? selection_end : end;
                int column_a = text_column_at(document, a);
                int column_b = text_column_at(document, b);
                if (b == end && selection_end > end)
                    column_b++;
                fill(editor,
                     (SDL_FRect){settings->gutter_width + settings->padding +
                                     column_a * editor->char_width,
                                 y, (column_b - column_a) * editor->char_width,
                                 editor->line_height},
                     selection);
            }
            if (is_c_buffer(buffer))
                draw_highlighted_line(editor, document->text + start, end - start,
                                      settings->gutter_width + settings->padding, y);
            else
                draw_text(editor, document->text + start, end - start,
                          settings->gutter_width + settings->padding, y, foreground);
        }
        if (end == document->length)
            break;
        start = end + 1;
        line++;
    }

    int cursor_line = text_line_at(document, document->cursor);
    if (cursor_line >= first && cursor_line <= last &&
        (SDL_GetTicks() / 500) % 2 == 0) {
        int column = text_column_at(document, document->cursor);
        fill(editor,
             (SDL_FRect){settings->gutter_width + settings->padding +
                             column * editor->char_width,
                         settings->top_height +
                             (cursor_line - first) * editor->line_height,
                         2, editor->line_height},
             accent);
    }

    char status[1400];
    if (editor->minibuffer.mode != MINIBUFFER_INACTIVE)
        snprintf(status, sizeof(status), "%s%s", editor->minibuffer.prompt,
                 editor->minibuffer.input);
    else
        snprintf(status, sizeof(status), "%s  |  Ln %d, Col %d  |  M-x comandos  |  %s",
                 buffer->type == BUFFER_DIRECTORY ? "dired"
                 : is_c_buffer(buffer)            ? "C"
                                                  : "text",
                 cursor_line + 1, text_column_at(document, document->cursor) + 1,
                 editor->message);
    draw_text(editor, status, strlen(status), settings->padding,
              editor->height - settings->status_height + 4,
              editor->minibuffer.mode == MINIBUFFER_INACTIVE ? muted : foreground);
    SDL_RenderPresent(editor->renderer);
}
