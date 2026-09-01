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
    /** Registers one package-owned command in the editor's unified registry. */
    bool (*register_command)(struct Editor *, const char *, const char *, unsigned int,
                             CommandFunction);
} MtAPI;

/** Initializes a package through the version-current editor API. */
typedef bool (*MtPackageInit)(MtAPI *api);

/** Initializes an empty package-handle manager. */
void packages_init(PackageManager *manager);
/** Closes every dynamic-library handle owned by the manager. */
void packages_destroy(PackageManager *manager);
/** Loads packages from path and returns the number initialized successfully. */
int packages_load_directory(PackageManager *manager, struct Editor *editor,
                            const char *path, char *message, size_t message_size);

#endif
