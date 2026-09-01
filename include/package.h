#ifndef MT_PACKAGE_H
#define MT_PACKAGE_H

#include <stdbool.h>
#include <stddef.h>

struct Editor;
typedef void (*PackageCommand)(struct Editor *editor, bool selecting);

typedef struct {
    char name[64];
    PackageCommand function;
} RegisteredCommand;

#define MT_MAX_COMMANDS 128
#define MT_MAX_PACKAGES 32

typedef struct {
    RegisteredCommand commands[MT_MAX_COMMANDS];
    size_t command_count;
    void *handles[MT_MAX_PACKAGES];
    size_t handle_count;
} PackageManager;

typedef struct {
    struct Editor *editor;
    bool (*register_command)(struct Editor *, const char *, PackageCommand);
} MtAPI;

typedef bool (*MtPackageInit)(MtAPI *api);

void packages_init(PackageManager *manager);
void packages_destroy(PackageManager *manager);
bool packages_register(PackageManager *manager, const char *name,
                       PackageCommand function);
PackageCommand packages_find(const PackageManager *manager, const char *name);
int packages_load_directory(PackageManager *manager, struct Editor *editor,
                            const char *path, char *message, size_t message_size);

#endif
