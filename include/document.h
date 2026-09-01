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
/** Initializes an empty document and its revision history. */
bool document_init(Document *document);
/** Releases text, path, and history owned by the document. */
void document_destroy(Document *document);
/** Replaces the document from path and resets its clean revision state. */
bool document_load(Document *document, const char *path, char *message, size_t size);
/** Atomically persists the document while preserving existing file permissions. */
bool document_save(Document *document, char *message, size_t size);
/** Replaces the selection with text as an independent undoable edit. */
bool document_insert(Document *document, const char *text);
/** Inserts typed text, allowing adjacent typed inserts to share an undo group. */
bool document_insert_typed(Document *document, const char *text);
/** Erases the clamped byte range and records the mutation for undo. */
void document_erase(Document *document, size_t start, size_t end);
/** Applies the latest undo edit and reports whether history was available. */
bool document_undo(Document *document);
/** Reapplies the latest redo edit and reports whether history was available. */
bool document_redo(Document *document);
/** Discards undo and redo history without changing document text. */
void document_clear_history(Document *document);
/** Prevents subsequent typed input from merging with the current undo group. */
void document_break_undo_group(Document *document);
/** Records the current revision as saved and clears the dirty flag. */
void document_mark_clean(Document *document);
/** Returns the lower byte offset of the cursor-anchor selection. */
size_t document_selection_start(const Document *document);
/** Returns the upper byte offset of the cursor-anchor selection. */
size_t document_selection_end(const Document *document);
/** Reports whether cursor and anchor delimit a non-empty selection. */
bool document_has_selection(const Document *document);
#endif
