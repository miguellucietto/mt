#ifndef MT_TEXT_H
#define MT_TEXT_H
#include "document.h"
/** Returns the byte offset of the UTF-8 codepoint preceding position. */
size_t text_previous_codepoint(const char *text, size_t position);
/** Returns the byte offset after the UTF-8 codepoint at position. */
size_t text_next_codepoint(const char *text, size_t length, size_t position);
/** Returns the first byte offset of the line containing position. */
size_t text_line_start(const Document *document, size_t position);
/** Returns the newline or document-end offset for the containing line. */
size_t text_line_end(const Document *document, size_t position);
/** Returns the zero-based logical line containing a byte position. */
int text_line_at(const Document *document, size_t position);
/** Returns the UTF-8 codepoint column at a byte position. */
int text_column_at(const Document *document, size_t position);
/** Maps a logical line and codepoint column to a document byte offset. */
size_t text_position_at(const Document *document, int line, int column);
#endif
