#ifndef MT_CONFIG_H
#define MT_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define MT_CONFIG_PATH_SIZE 4096

typedef struct {
    char base_directory[MT_CONFIG_PATH_SIZE];
    char directory[MT_CONFIG_PATH_SIZE];
    char keymap_path[MT_CONFIG_PATH_SIZE];
    char settings_path[MT_CONFIG_PATH_SIZE];
    char packages_path[MT_CONFIG_PATH_SIZE];
} ConfigPaths;

/** Derives all configuration paths from an explicit base without filesystem writes. */
bool config_paths_from_base(ConfigPaths *paths, const char *base, char *message,
                            size_t message_size);
/** Discovers the XDG or HOME configuration base without filesystem writes. */
bool config_paths_discover(ConfigPaths *paths, char *message, size_t message_size);
/** Creates required directories and the default keymap without replacing user files. */
bool config_paths_prepare(const ConfigPaths *paths, char *message, size_t message_size);

#endif
