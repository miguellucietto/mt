#ifndef MT_PACKAGE_H
#define MT_PACKAGE_H

#include "command.h"

#include <stdbool.h>
#include <stddef.h>

struct Editor;
#define MT_MAX_PACKAGES 32

typedef struct {
    void *handles[MT_MAX_PACKAGES];
    size_t handle_count;
} PackageManager;

typedef struct {
    struct Editor *editor;
    bool (*register_command)(struct Editor *, const char *, const char *, unsigned int,
                             CommandFunction);
} MtAPI;

typedef bool (*MtPackageInit)(MtAPI *api);

void packages_init(PackageManager *manager);
void packages_destroy(PackageManager *manager);
int packages_load_directory(PackageManager *manager, struct Editor *editor,
                            const char *path, char *message, size_t message_size);

#endif
