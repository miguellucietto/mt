#include "buffer.h"
#include "text.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Returns a borrowed pointer to the final component of path. */
static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash && slash[1] ? slash + 1 : path;
}

/* Compares canonical paths when possible and falls back to literal equality. */
static bool same_file(const char *left, const char *right)
{
    if (!left || !right)
        return false;
    if (strcmp(left, right) == 0)
        return true;
    struct stat left_info, right_info;
    return stat(left, &left_info) == 0 && stat(right, &right_info) == 0 &&
           left_info.st_dev == right_info.st_dev &&
           left_info.st_ino == right_info.st_ino;
}

/* Finds a same-name buffer or returns manager->count when none exists. */
static size_t buffer_index_for_name(const BufferManager *manager, const char *name)
{
    for (size_t i = 0; i < manager->count; i++)
        if (strcmp(manager->items[i].name, name) == 0)
            return i;
    return manager->count;
}

bool buffers_init(BufferManager *manager)
{
    memset(manager, 0, sizeof(*manager));
    return buffers_open_text(manager, "*scratch*", BUFFER_TEXT, "", false) != NULL;
}

void buffers_destroy(BufferManager *manager)
{
    for (size_t i = 0; i < manager->count; i++)
        document_destroy(&manager->items[i].document);
    memset(manager, 0, sizeof(*manager));
}

Buffer *buffers_current(BufferManager *manager)
{
    return manager->count ? &manager->items[manager->active] : NULL;
}

const Buffer *buffers_current_const(const BufferManager *manager)
{
    return manager->count ? &manager->items[manager->active] : NULL;
}

Buffer *buffers_create(BufferManager *manager, const char *name, BufferType type)
{
    for (size_t i = 0; i < manager->count; i++) {
        if (strcmp(manager->items[i].name, name) == 0) {
            manager->active = i;
            return &manager->items[i];
        }
    }
    if (manager->count >= MT_MAX_BUFFERS)
        return NULL;
    Buffer *buffer = &manager->items[manager->count];
    memset(buffer, 0, sizeof(*buffer));
    if (!document_init(&buffer->document))
        return NULL;
    snprintf(buffer->name, sizeof(buffer->name), "%s", name);
    buffer->type = type;
    manager->active = manager->count++;
    return buffer;
}

bool buffers_file_would_replace_modified(const BufferManager *manager, const char *path)
{
    size_t index = buffer_index_for_name(manager, base_name(path));
    if (index == manager->count)
        return false;
    const Buffer *buffer = &manager->items[index];
    return buffer->document.dirty && !same_file(buffer->document.path, path);
}

size_t buffers_modified_count(const BufferManager *manager)
{
    size_t count = 0;
    for (size_t i = 0; i < manager->count; i++)
        if (manager->items[i].document.dirty)
            count++;
    return count;
}

/* Loads path into a reusable same-name buffer, honoring explicit replacement. */
static Buffer *open_file(BufferManager *manager, const char *path, char *message,
                         size_t message_size, bool replace_modified)
{
    struct stat info;
    if (stat(path, &info) == 0 && S_ISDIR(info.st_mode)) {
        Buffer *buffer = buffers_create(manager, "*dired*", BUFFER_DIRECTORY);
        return buffer && buffer_refresh_directory(buffer, path, message, message_size)
                   ? buffer
                   : NULL;
    }
    char name[MT_BUFFER_NAME_SIZE];
    snprintf(name, sizeof(name), "%s", base_name(path));
    size_t existing = buffer_index_for_name(manager, name);
    if (existing < manager->count) {
        Buffer *buffer = &manager->items[existing];
        manager->active = existing;
        if (same_file(buffer->document.path, path)) {
            snprintf(message, message_size, "Buffer já aberto: %s", path);
            return buffer;
        }
        if (buffer->document.dirty && !replace_modified) {
            snprintf(message, message_size, "Buffer modificado: %s", buffer->name);
            return NULL;
        }
    }
    Buffer *buffer = buffers_create(manager, name, BUFFER_TEXT);
    if (!buffer)
        return NULL;
    if (!document_load(&buffer->document, path, message, message_size))
        return NULL;
    return buffer;
}

Buffer *buffers_open_file(BufferManager *manager, const char *path, char *message,
                          size_t message_size)
{
    return open_file(manager, path, message, message_size, false);
}

Buffer *buffers_open_file_confirmed(BufferManager *manager, const char *path,
                                    char *message, size_t message_size)
{
    return open_file(manager, path, message, message_size, true);
}

Buffer *buffers_open_text(BufferManager *manager, const char *name, BufferType type,
                          const char *text, bool read_only)
{
    Buffer *buffer = buffers_create(manager, name, type);
    if (!buffer)
        return NULL;
    buffer->read_only = false;
    buffer->document.anchor = 0;
    buffer->document.cursor = buffer->document.length;
    document_erase(&buffer->document, 0, buffer->document.length);
    if (!document_insert(&buffer->document, text))
        return NULL;
    buffer->document.cursor = buffer->document.anchor = 0;
    document_clear_history(&buffer->document);
    document_mark_clean(&buffer->document);
    buffer->read_only = read_only;
    return buffer;
}

void buffers_next(BufferManager *manager)
{
    if (manager->count)
        manager->active = (manager->active + 1) % manager->count;
}

bool buffer_refresh_directory(Buffer *buffer, const char *path, char *message,
                              size_t message_size)
{
    struct dirent **entries = NULL;
    int count = scandir(path, &entries, NULL, alphasort);
    if (count < 0) {
        snprintf(message, message_size, "Não foi possível abrir: %s", path);
        return false;
    }
    buffer->read_only = false;
    buffer->document.anchor = 0;
    buffer->document.cursor = buffer->document.length;
    document_erase(&buffer->document, 0, buffer->document.length);
    snprintf(buffer->directory, sizeof(buffer->directory), "%s", path);
    char header[MT_PATH_SIZE + 32];
    snprintf(header, sizeof(header), "Diretório: %s\n\n", path);
    document_insert(&buffer->document, header);
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i]->d_name, ".") == 0) {
            free(entries[i]);
            continue;
        }
        char full_path[MT_PATH_SIZE], line[MT_PATH_SIZE + 8];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i]->d_name);
        struct stat info;
        bool directory = stat(full_path, &info) == 0 && S_ISDIR(info.st_mode);
        snprintf(line, sizeof(line), "%s %s\n", directory ? "[D]" : "   ",
                 entries[i]->d_name);
        document_insert(&buffer->document, line);
        free(entries[i]);
    }
    free(entries);
    buffer->document.cursor = buffer->document.anchor = 0;
    document_clear_history(&buffer->document);
    document_mark_clean(&buffer->document);
    buffer->read_only = true;
    buffer->type = BUFFER_DIRECTORY;
    snprintf(message, message_size, "Dired: Enter abre, g atualiza");
    return true;
}

bool buffer_directory_entry(const Buffer *buffer, int line, char *path, size_t size,
                            bool *is_directory)
{
    if (buffer->type != BUFFER_DIRECTORY || line < 2)
        return false;
    size_t start = text_position_at(&buffer->document, line, 0);
    size_t end = text_line_end(&buffer->document, start);
    if (end - start < 4)
        return false;
    *is_directory = buffer->document.text[start] == '[';
    snprintf(path, size, "%s/%.*s", buffer->directory, (int)(end - start - 4),
             buffer->document.text + start + 4);
    return true;
}
