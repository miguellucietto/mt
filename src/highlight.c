#include "highlight.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/* Tests an exact byte range against the supported C keyword set. */
static bool is_keyword(const char *word, size_t length)
{
    static const char *keywords[] = {
        "auto",  "break",    "case",   "char",     "const",  "continue", "default",
        "do",    "double",   "else",   "enum",     "extern", "float",    "for",
        "goto",  "if",       "inline", "int",      "long",   "register", "return",
        "short", "signed",   "sizeof", "static",   "struct", "switch",   "typedef",
        "union", "unsigned", "void",   "volatile", "while",  "_Bool"};
    for (size_t i = 0; i < sizeof(keywords) / sizeof(*keywords); i++)
        if (strlen(keywords[i]) == length && strncmp(word, keywords[i], length) == 0)
            return true;
    return false;
}

/* Appends a non-empty lexical span when output capacity remains. */
static void add_span(HighlightSpan *spans, size_t capacity, size_t *count, size_t start,
                     size_t length, HighlightKind kind)
{
    if (*count < capacity)
        spans[(*count)++] = (HighlightSpan){start, length, kind};
}

size_t highlight_c_line(const char *text, size_t length, HighlightSpan *spans,
                        size_t capacity)
{
    size_t count = 0, position = 0;
    while (position < length) {
        if (text[position] == '/' && position + 1 < length &&
            text[position + 1] == '/') {
            add_span(spans, capacity, &count, position, length - position,
                     HIGHLIGHT_COMMENT);
            break;
        }
        if (text[position] == '#' &&
            (position == 0 || isspace((unsigned char)text[position - 1]))) {
            add_span(spans, capacity, &count, position, length - position,
                     HIGHLIGHT_PREPROCESSOR);
            break;
        }
        if (text[position] == '"' || text[position] == '\'') {
            char quote = text[position];
            size_t start = position++;
            while (position < length) {
                if (text[position] == '\\' && position + 1 < length)
                    position += 2;
                else if (text[position++] == quote)
                    break;
                else
                    continue;
            }
            add_span(spans, capacity, &count, start, position - start,
                     HIGHLIGHT_STRING);
            continue;
        }
        if (isdigit((unsigned char)text[position])) {
            size_t start = position++;
            while (position < length &&
                   (isalnum((unsigned char)text[position]) || text[position] == '.'))
                position++;
            add_span(spans, capacity, &count, start, position - start,
                     HIGHLIGHT_NUMBER);
            continue;
        }
        if (isalpha((unsigned char)text[position]) || text[position] == '_') {
            size_t start = position++;
            while (position < length &&
                   (isalnum((unsigned char)text[position]) || text[position] == '_'))
                position++;
            if (is_keyword(text + start, position - start))
                add_span(spans, capacity, &count, start, position - start,
                         HIGHLIGHT_KEYWORD);
            continue;
        }
        position++;
    }
    return count;
}

SDL_Color highlight_color(HighlightKind kind)
{
    switch (kind) {
    case HIGHLIGHT_KEYWORD:
        return (SDL_Color){198, 120, 221, 255};
    case HIGHLIGHT_STRING:
        return (SDL_Color){152, 195, 121, 255};
    case HIGHLIGHT_COMMENT:
        return (SDL_Color){92, 99, 112, 255};
    case HIGHLIGHT_NUMBER:
        return (SDL_Color){209, 154, 102, 255};
    case HIGHLIGHT_PREPROCESSOR:
        return (SDL_Color){86, 182, 194, 255};
    case HIGHLIGHT_NORMAL:
        return (SDL_Color){224, 226, 235, 255};
    }
    return (SDL_Color){224, 226, 235, 255};
}
