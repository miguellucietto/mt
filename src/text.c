#include "text.h"
size_t text_previous_codepoint(const char *text, size_t position)
{
    if (!position)
        return 0;
    position--;
    while (position && ((unsigned char)text[position] & 0xc0) == 0x80)
        position--;
    return position;
}
size_t text_next_codepoint(const char *text, size_t length, size_t position)
{
    if (position >= length)
        return length;
    position++;
    while (position < length && ((unsigned char)text[position] & 0xc0) == 0x80)
        position++;
    return position;
}
size_t text_line_start(const Document *d, size_t p)
{
    while (p && d->text[p - 1] != '\n')
        p--;
    return p;
}
size_t text_line_end(const Document *d, size_t p)
{
    while (p < d->length && d->text[p] != '\n')
        p++;
    return p;
}
int text_line_at(const Document *d, size_t p)
{
    int line = 0;
    for (size_t i = 0; i < p; i++)
        if (d->text[i] == '\n')
            line++;
    return line;
}
int text_column_at(const Document *d, size_t p)
{
    int column = 0;
    for (size_t i = text_line_start(d, p); i < p; column++)
        i = text_next_codepoint(d->text, p, i);
    return column;
}
size_t text_position_at(const Document *d, int target, int column)
{
    size_t p = 0;
    int line = 0;
    while (p < d->length && line < target)
        if (d->text[p++] == '\n')
            line++;
    while (p < d->length && d->text[p] != '\n' && column-- > 0)
        p = text_next_codepoint(d->text, d->length, p);
    return p;
}
