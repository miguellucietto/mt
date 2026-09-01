#ifndef MT_COMMAND_H
#define MT_COMMAND_H
#include <stdbool.h>
#include <stddef.h>

struct Editor;

#define MT_MAX_COMMANDS 128
#define MT_COMMAND_NAME_SIZE 64
#define MT_COMMAND_DESCRIPTION_SIZE 160

typedef void (*CommandFunction)(struct Editor *editor, bool selecting);

typedef enum {
    COMMAND_FLAG_NONE = 0,
    COMMAND_FLAG_OPENS_MINIBUFFER = 1 << 0,
    COMMAND_FLAG_KEEP_COLUMN = 1 << 1
} CommandFlags;

typedef struct {
    char name[MT_COMMAND_NAME_SIZE];
    char description[MT_COMMAND_DESCRIPTION_SIZE];
    CommandFunction function;
    unsigned int flags;
} CommandSpec;

typedef struct {
    CommandSpec commands[MT_MAX_COMMANDS];
    size_t count;
} CommandRegistry;

void command_registry_init(CommandRegistry *registry);
bool command_registry_register(CommandRegistry *registry, const char *name,
                               const char *description, unsigned int flags,
                               CommandFunction function);
const CommandSpec *command_registry_find(const CommandRegistry *registry,
                                         const char *name);
const CommandSpec *command_registry_at(const CommandRegistry *registry, size_t index);
bool command_registry_execute(const CommandRegistry *registry, const char *name,
                              struct Editor *editor, bool selecting);
#endif
