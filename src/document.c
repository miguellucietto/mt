#include "document.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INITIAL_CAPACITY 4096
static char *string_copy(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy)
        memcpy(copy, text, size);
    return copy;
}
static bool reserve(Document *d, size_t needed)
{
    if (needed + 1 <= d->capacity)
        return true;
    size_t capacity = d->capacity;
    while (capacity < needed + 1)
        capacity *= 2;
    char *text = realloc(d->text, capacity);
    if (!text)
        return false;
    d->text = text;
    d->capacity = capacity;
    return true;
}
bool document_init(Document *d)
{
    memset(d, 0, sizeof(*d));
    d->capacity = INITIAL_CAPACITY;
    d->text = calloc(d->capacity, 1);
    return d->text != NULL;
}
void document_destroy(Document *d)
{
    free(d->path);
    free(d->text);
    memset(d, 0, sizeof(*d));
}
size_t document_selection_start(const Document *d)
{
    return d->cursor < d->anchor ? d->cursor : d->anchor;
}
size_t document_selection_end(const Document *d)
{
    return d->cursor > d->anchor ? d->cursor : d->anchor;
}
bool document_has_selection(const Document *d)
{
    return d->cursor != d->anchor;
}
void document_erase(Document *d, size_t start, size_t end)
{
    if (start >= end || end > d->length)
        return;
    memmove(d->text + start, d->text + end, d->length - end + 1);
    d->length -= end - start;
    d->cursor = d->anchor = start;
    d->dirty = true;
}
bool document_insert(Document *d, const char *text)
{
    size_t start = document_selection_start(d), end = document_selection_end(d),
           inserted = strlen(text);
    size_t length = d->length - (end - start) + inserted;
    if (!reserve(d, length))
        return false;
    memmove(d->text + start + inserted, d->text + end, d->length - end + 1);
    memcpy(d->text + start, text, inserted);
    d->length = length;
    d->cursor = d->anchor = start + inserted;
    d->dirty = true;
    return true;
}
bool document_load(Document *d, const char *path, char *message, size_t size)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        if (errno != ENOENT) {
            snprintf(message, size, "Erro ao abrir: %s", strerror(errno));
            return false;
        }
        free(d->path);
        d->path = string_copy(path);
        snprintf(message, size, "Novo arquivo: %s", path);
        return d->path != NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long file_size = ftell(file);
    if (file_size < 0 || !reserve(d, (size_t)file_size)) {
        fclose(file);
        return false;
    }
    rewind(file);
    size_t read = fread(d->text, 1, (size_t)file_size, file);
    bool ok = !ferror(file);
    fclose(file);
    if (!ok)
        return false;
    d->text[read] = '\0';
    d->length = read;
    d->cursor = d->anchor = 0;
    d->dirty = false;
    free(d->path);
    d->path = string_copy(path);
    snprintf(message, size, "%s carregado", path);
    return d->path != NULL;
}
bool document_save(Document *d, char *message, size_t size)
{
    if (!d->path) {
        snprintf(message, size, "Abra com: ./mt arquivo.txt");
        return false;
    }
    FILE *file = fopen(d->path, "wb");
    if (!file) {
        snprintf(message, size, "Erro ao salvar: %s", strerror(errno));
        return false;
    }
    bool ok = fwrite(d->text, 1, d->length, file) == d->length;
    if (fclose(file) != 0)
        ok = false;
    if (ok)
        d->dirty = false;
    snprintf(message, size, ok ? "Salvo: %s" : "Erro ao salvar: %s", d->path);
    return ok;
}
