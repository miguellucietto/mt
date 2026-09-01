#ifndef MT_DOCUMENT_H
#define MT_DOCUMENT_H
#include <stdbool.h>
#include <stddef.h>

typedef struct DocumentEdit DocumentEdit;

typedef struct {
    char *path;
    char *text;
    size_t length, capacity, cursor, anchor;
    DocumentEdit *undo_edits, *redo_edits;
    size_t undo_count, undo_capacity, redo_count, redo_capacity;
    size_t revision, saved_revision, next_revision;
    bool dirty;
} Document;
bool document_init(Document *document);
void document_destroy(Document *document);
bool document_load(Document *document, const char *path, char *message, size_t size);
bool document_save(Document *document, char *message, size_t size);
bool document_insert(Document *document, const char *text);
bool document_insert_typed(Document *document, const char *text);
void document_erase(Document *document, size_t start, size_t end);
bool document_undo(Document *document);
bool document_redo(Document *document);
void document_clear_history(Document *document);
void document_break_undo_group(Document *document);
void document_mark_clean(Document *document);
size_t document_selection_start(const Document *document);
size_t document_selection_end(const Document *document);
bool document_has_selection(const Document *document);
#endif
