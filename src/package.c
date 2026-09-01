#include "package.h"
#include "editor.h"

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

void packages_init(PackageManager *manager)
{
    memset(manager, 0, sizeof(*manager));
}

void packages_destroy(PackageManager *manager)
{
    for (size_t i = 0; i < manager->handle_count; i++)
        dlclose(manager->handles[i]);
    memset(manager, 0, sizeof(*manager));
}

bool packages_register(PackageManager *manager, const char *name,
                       PackageCommand function)
{
    for (size_t i = 0; i < manager->command_count; i++) {
        if (strcmp(manager->commands[i].name, name) == 0) {
            manager->commands[i].function = function;
            return true;
        }
    }
    if (manager->command_count >= MT_MAX_COMMANDS || !function)
        return false;
    RegisteredCommand *command = &manager->commands[manager->command_count++];
    snprintf(command->name, sizeof(command->name), "%s", name);
    command->function = function;
    return true;
}

PackageCommand packages_find(const PackageManager *manager, const char *name)
{
    for (size_t i = 0; i < manager->command_count; i++)
        if (strcmp(manager->commands[i].name, name) == 0)
            return manager->commands[i].function;
    return NULL;
}

int packages_load_directory(PackageManager *manager, struct Editor *editor,
                            const char *path, char *message, size_t message_size)
{
    DIR *directory = opendir(path);
    if (!directory)
        return 0;
    int loaded = 0;
    struct dirent *entry;
    while ((entry = readdir(directory))) {
        size_t length = strlen(entry->d_name);
        if (length < 4 || strcmp(entry->d_name + length - 3, ".so") != 0 ||
            manager->handle_count >= MT_MAX_PACKAGES)
            continue;
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        void *handle = dlopen(full_path, RTLD_NOW | RTLD_LOCAL);
        if (!handle)
            continue;
        MtPackageInit initialize = NULL;
        void *symbol = dlsym(handle, "mt_package_init");
        memcpy(&initialize, &symbol, sizeof(initialize));
        MtAPI api = {.editor = editor, .register_command = editor_register_command};
        if (!initialize || !initialize(&api)) {
            dlclose(handle);
            continue;
        }
        manager->handles[manager->handle_count++] = handle;
        loaded++;
    }
    closedir(directory);
    if (loaded)
        snprintf(message, message_size, "%d package(s) carregado(s)", loaded);
    return loaded;
}
