#ifndef MT_BUFFER_H
#define MT_BUFFER_H

#include "document.h"

#include <stdbool.h>
#include <stddef.h>

#define MT_MAX_BUFFERS 32
#define MT_BUFFER_NAME_SIZE 128
#define MT_PATH_SIZE 4096

typedef enum {
    BUFFER_TEXT,
    BUFFER_SHELL,
    BUFFER_DIRECTORY,
    BUFFER_MESSAGES
} BufferType;

typedef struct {
    char name[MT_BUFFER_NAME_SIZE];
    char directory[MT_PATH_SIZE];
    BufferType type;
    Document document;
    bool read_only;
} Buffer;

typedef struct {
    Buffer items[MT_MAX_BUFFERS];
    size_t count;
    size_t active;
} BufferManager;

bool buffers_init(BufferManager *manager);
void buffers_destroy(BufferManager *manager);
Buffer *buffers_current(BufferManager *manager);
const Buffer *buffers_current_const(const BufferManager *manager);
Buffer *buffers_create(BufferManager *manager, const char *name, BufferType type);
Buffer *buffers_open_file(BufferManager *manager, const char *path, char *message,
                          size_t message_size);
Buffer *buffers_open_text(BufferManager *manager, const char *name, BufferType type,
                          const char *text, bool read_only);
void buffers_next(BufferManager *manager);
bool buffer_refresh_directory(Buffer *buffer, const char *path, char *message,
                              size_t message_size);
bool buffer_directory_entry(const Buffer *buffer, int line, char *path, size_t size,
                            bool *is_directory);

#endif
