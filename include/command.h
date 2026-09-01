#ifndef MT_COMMAND_H
#define MT_COMMAND_H
#include <stdbool.h>
#include <stddef.h>

struct Editor;

#define MT_MAX_COMMANDS 128
#define MT_COMMAND_NAME_SIZE 64
#define MT_COMMAND_DESCRIPTION_SIZE 160

/** Implements a registered command against editor interaction state. */
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

/** Resets a registry to an empty, usable state. */
void command_registry_init(CommandRegistry *registry);
/** Adds a validated unique command without modifying the registry on failure. */
bool command_registry_register(CommandRegistry *registry, const char *name,
                               const char *description, unsigned int flags,
                               CommandFunction function);
/** Returns the command named name, or NULL when it is not registered. */
const CommandSpec *command_registry_find(const CommandRegistry *registry,
                                         const char *name);
/** Returns the command at index, or NULL when index is outside the registry. */
const CommandSpec *command_registry_at(const CommandRegistry *registry, size_t index);
/** Executes a named command and reports whether it was found and invoked. */
bool command_registry_execute(const CommandRegistry *registry, const char *name,
                              struct Editor *editor, bool selecting);
#endif
