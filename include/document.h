#ifndef MT_DOCUMENT_H
#define MT_DOCUMENT_H
#include <stdbool.h>
#include <stddef.h>
typedef struct {
    char *path;
    char *text;
    size_t length, capacity, cursor, anchor;
    bool dirty;
} Document;
bool document_init(Document *document);
void document_destroy(Document *document);
bool document_load(Document *document, const char *path, char *message, size_t size);
bool document_save(Document *document, char *message, size_t size);
bool document_insert(Document *document, const char *text);
void document_erase(Document *document, size_t start, size_t end);
size_t document_selection_start(const Document *document);
size_t document_selection_end(const Document *document);
bool document_has_selection(const Document *document);
#endif
