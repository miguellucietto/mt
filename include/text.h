#ifndef MT_TEXT_H
#define MT_TEXT_H
#include "document.h"
size_t text_previous_codepoint(const char *text, size_t position);
size_t text_next_codepoint(const char *text, size_t length, size_t position);
size_t text_line_start(const Document *document, size_t position);
size_t text_line_end(const Document *document, size_t position);
int text_line_at(const Document *document, size_t position);
int text_column_at(const Document *document, size_t position);
size_t text_position_at(const Document *document, int line, int column);
#endif
