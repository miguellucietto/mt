#include "document.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INITIAL_CAPACITY 4096

struct DocumentEdit {
    size_t position;
    char *removed;
    size_t removed_length;
    char *inserted;
    size_t inserted_length;
    size_t cursor_before, anchor_before;
    size_t cursor_after, anchor_after;
    size_t revision_before, revision_after;
    bool typed;
};

static char *string_copy(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy)
        memcpy(copy, text, size);
    return copy;
}

static char *memory_copy(const char *text, size_t length)
{
    char *copy = malloc(length + 1);
    if (!copy)
        return NULL;
    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

static void edit_destroy(DocumentEdit *edit)
{
    free(edit->removed);
    free(edit->inserted);
    memset(edit, 0, sizeof(*edit));
}

static void edits_clear(DocumentEdit *edits, size_t count)
{
    for (size_t i = 0; i < count; i++)
        edit_destroy(&edits[i]);
}

static bool edits_push(DocumentEdit **edits, size_t *count, size_t *capacity,
                       DocumentEdit edit)
{
    if (*count == *capacity) {
        size_t new_capacity = *capacity ? *capacity * 2 : 32;
        DocumentEdit *larger = realloc(*edits, new_capacity * sizeof(*larger));
        if (!larger)
            return false;
        *edits = larger;
        *capacity = new_capacity;
    }
    (*edits)[(*count)++] = edit;
    return true;
}

static void update_dirty(Document *d)
{
    d->dirty = d->revision != d->saved_revision;
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
    document_clear_history(d);
    free(d->undo_edits);
    free(d->redo_edits);
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
static bool replace(Document *d, size_t start, size_t end, const char *text,
                    size_t inserted, bool typed)
{
    if (start > end || end > d->length)
        return false;
    size_t removed = end - start;
    size_t length = d->length - removed + inserted;
    DocumentEdit edit = {.position = start,
                         .removed_length = removed,
                         .inserted_length = inserted,
                         .cursor_before = d->cursor,
                         .anchor_before = d->anchor,
                         .revision_before = d->revision,
                         .revision_after = ++d->next_revision,
                         .typed = typed};
    edit.removed = memory_copy(d->text + start, removed);
    edit.inserted = memory_copy(text, inserted);
    if (!edit.removed || !edit.inserted || !reserve(d, length) ||
        !edits_push(&d->undo_edits, &d->undo_count, &d->undo_capacity, edit)) {
        edit_destroy(&edit);
        return false;
    }
    edits_clear(d->redo_edits, d->redo_count);
    d->redo_count = 0;
    memmove(d->text + start + inserted, d->text + end, d->length - end + 1);
    memcpy(d->text + start, text, inserted);
    d->length = length;
    d->cursor = d->anchor = start + inserted;
    DocumentEdit *stored = &d->undo_edits[d->undo_count - 1];
    stored->cursor_after = d->cursor;
    stored->anchor_after = d->anchor;
    d->revision = stored->revision_after;
    update_dirty(d);
    return true;
}

static void merge_typed_insert(Document *d)
{
    if (d->undo_count < 2)
        return;
    DocumentEdit *first = &d->undo_edits[d->undo_count - 2];
    DocumentEdit *second = &d->undo_edits[d->undo_count - 1];
    if (!first->typed || !second->typed || first->removed_length ||
        second->removed_length ||
        first->position + first->inserted_length != second->position ||
        first->cursor_after != second->cursor_before ||
        first->anchor_after != second->anchor_before)
        return;
    char *joined =
        realloc(first->inserted, first->inserted_length + second->inserted_length + 1);
    if (!joined)
        return;
    memcpy(joined + first->inserted_length, second->inserted,
           second->inserted_length + 1);
    first->inserted = joined;
    first->inserted_length += second->inserted_length;
    first->cursor_after = second->cursor_after;
    first->anchor_after = second->anchor_after;
    first->revision_after = second->revision_after;
    edit_destroy(second);
    d->undo_count--;
}

static bool insert(Document *d, const char *text, bool typed)
{
    size_t start = document_selection_start(d), end = document_selection_end(d);
    if (!replace(d, start, end, text, strlen(text), typed))
        return false;
    if (typed)
        merge_typed_insert(d);
    return true;
}

bool document_insert(Document *d, const char *text)
{
    return insert(d, text, false);
}

bool document_insert_typed(Document *d, const char *text)
{
    return insert(d, text, true);
}

void document_erase(Document *d, size_t start, size_t end)
{
    if (start < end && end <= d->length)
        (void)replace(d, start, end, "", 0, false);
}

static bool apply_history(Document *d, DocumentEdit **source, size_t *source_count,
                          DocumentEdit **target, size_t *target_count,
                          size_t *target_capacity, bool redo)
{
    if (!*source_count)
        return false;
    DocumentEdit edit = (*source)[*source_count - 1];
    size_t replaced = redo ? edit.removed_length : edit.inserted_length;
    const char *text = redo ? edit.inserted : edit.removed;
    size_t inserted = redo ? edit.inserted_length : edit.removed_length;
    size_t length = d->length - replaced + inserted;
    if (!reserve(d, length) || !edits_push(target, target_count, target_capacity, edit))
        return false;
    (*source_count)--;
    memmove(d->text + edit.position + inserted, d->text + edit.position + replaced,
            d->length - edit.position - replaced + 1);
    memcpy(d->text + edit.position, text, inserted);
    d->length = length;
    d->cursor = redo ? edit.cursor_after : edit.cursor_before;
    d->anchor = redo ? edit.anchor_after : edit.anchor_before;
    d->revision = redo ? edit.revision_after : edit.revision_before;
    update_dirty(d);
    return true;
}

bool document_undo(Document *d)
{
    return apply_history(d, &d->undo_edits, &d->undo_count, &d->redo_edits,
                         &d->redo_count, &d->redo_capacity, false);
}

bool document_redo(Document *d)
{
    return apply_history(d, &d->redo_edits, &d->redo_count, &d->undo_edits,
                         &d->undo_count, &d->undo_capacity, true);
}

void document_clear_history(Document *d)
{
    edits_clear(d->undo_edits, d->undo_count);
    edits_clear(d->redo_edits, d->redo_count);
    d->undo_count = d->redo_count = 0;
}

void document_break_undo_group(Document *d)
{
    if (d->undo_count)
        d->undo_edits[d->undo_count - 1].typed = false;
}

void document_mark_clean(Document *d)
{
    document_break_undo_group(d);
    d->saved_revision = d->revision;
    update_dirty(d);
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
    document_clear_history(d);
    d->revision = d->saved_revision = d->next_revision = 0;
    update_dirty(d);
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
        document_mark_clean(d);
    snprintf(message, size, ok ? "Salvo: %s" : "Erro ao salvar: %s", d->path);
    return ok;
}
