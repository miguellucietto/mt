#ifndef MT_HIGHLIGHT_H
#define MT_HIGHLIGHT_H

#include <SDL3/SDL.h>
#include <stddef.h>

typedef enum {
    HIGHLIGHT_NORMAL,
    HIGHLIGHT_KEYWORD,
    HIGHLIGHT_STRING,
    HIGHLIGHT_COMMENT,
    HIGHLIGHT_NUMBER,
    HIGHLIGHT_PREPROCESSOR
} HighlightKind;

typedef struct {
    size_t start, length;
    HighlightKind kind;
} HighlightSpan;

size_t highlight_c_line(const char *text, size_t length, HighlightSpan *spans,
                        size_t capacity);
SDL_Color highlight_color(HighlightKind kind);

#endif
