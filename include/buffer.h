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

/** Initializes a manager with one empty text buffer. */
bool buffers_init(BufferManager *manager);
/** Releases every document owned by the manager. */
void buffers_destroy(BufferManager *manager);
/** Returns the active buffer, or NULL when the manager is empty. */
Buffer *buffers_current(BufferManager *manager);
/** Returns a read-only view of the active buffer, or NULL when empty. */
const Buffer *buffers_current_const(const BufferManager *manager);
/** Creates and activates a uniquely named buffer when capacity permits. */
Buffer *buffers_create(BufferManager *manager, const char *name, BufferType type);
/** Opens and activates a file unless that would replace modified contents. */
Buffer *buffers_open_file(BufferManager *manager, const char *path, char *message,
                          size_t message_size);
/** Opens a file while explicitly allowing replacement of modified contents. */
Buffer *buffers_open_file_confirmed(BufferManager *manager, const char *path,
                                    char *message, size_t message_size);
/** Reports whether opening path would replace a modified same-name buffer. */
bool buffers_file_would_replace_modified(const BufferManager *manager,
                                         const char *path);
/** Counts buffers whose documents contain unsaved changes. */
size_t buffers_modified_count(const BufferManager *manager);
/** Creates or reuses a named buffer and replaces its document with text. */
Buffer *buffers_open_text(BufferManager *manager, const char *name, BufferType type,
                          const char *text, bool read_only);
/** Activates the next buffer, wrapping at the end. */
void buffers_next(BufferManager *manager);
/** Rebuilds a directory buffer from path and reports failures in message. */
bool buffer_refresh_directory(Buffer *buffer, const char *path, char *message,
                              size_t message_size);
/** Resolves a rendered directory line into a path and entry type. */
bool buffer_directory_entry(const Buffer *buffer, int line, char *path, size_t size,
                            bool *is_directory);

#endif
