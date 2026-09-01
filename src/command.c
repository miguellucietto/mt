#include "command.h"

#include <string.h>

void command_registry_init(CommandRegistry *registry)
{
    registry->count = 0;
}

const CommandSpec *command_registry_find(const CommandRegistry *registry,
                                         const char *name)
{
    if (!name)
        return NULL;
    for (size_t i = 0; i < registry->count; i++)
        if (strcmp(registry->commands[i].name, name) == 0)
            return &registry->commands[i];
    return NULL;
}

bool command_registry_register(CommandRegistry *registry, const char *name,
                               const char *description, unsigned int flags,
                               CommandFunction function)
{
    if (!name || !*name || strlen(name) >= MT_COMMAND_NAME_SIZE || !description ||
        strlen(description) >= MT_COMMAND_DESCRIPTION_SIZE || !function ||
        registry->count >= MT_MAX_COMMANDS || command_registry_find(registry, name))
        return false;
    CommandSpec *command = &registry->commands[registry->count++];
    memcpy(command->name, name, strlen(name) + 1);
    memcpy(command->description, description, strlen(description) + 1);
    command->function = function;
    command->flags = flags;
    return true;
}

const CommandSpec *command_registry_at(const CommandRegistry *registry, size_t index)
{
    return index < registry->count ? &registry->commands[index] : NULL;
}

bool command_registry_execute(const CommandRegistry *registry, const char *name,
                              struct Editor *editor, bool selecting)
{
    const CommandSpec *command = command_registry_find(registry, name);
    if (!command)
        return false;
    command->function(editor, selecting);
    return true;
}
