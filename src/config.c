#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Writes a bounded configuration diagnostic and returns false for failure paths. */
static bool fail(char *message, size_t message_size, const char *reason,
                 const char *path)
{
    if (message && message_size)
        snprintf(message, message_size, "%s: %s", reason, path ? path : "");
    return false;
}

/* Joins two path components into bounded storage without accepting truncation. */
static bool join_path(char *destination, size_t size, const char *base,
                      const char *name)
{
    size_t base_length = strlen(base), name_length = strlen(name);
    if (base_length + name_length + 2 > size)
        return false;
    memcpy(destination, base, base_length);
    destination[base_length] = '/';
    memcpy(destination + base_length + 1, name, name_length + 1);
    return true;
}

/* Ensures path is a directory, creating only its final component when absent. */
static bool ensure_directory(const char *path, char *message, size_t message_size)
{
    if (mkdir(path, 0755) == 0)
        return true;
    if (errno != EEXIST)
        return fail(message, message_size, strerror(errno), path);
    struct stat information;
    if (stat(path, &information) != 0 || !S_ISDIR(information.st_mode))
        return fail(message, message_size, "Configuration path is not a directory",
                    path);
    return true;
}

bool config_paths_from_base(ConfigPaths *paths, const char *base, char *message,
                            size_t message_size)
{
    if (!paths || !base || !*base)
        return fail(message, message_size, "Invalid configuration base", base);
    ConfigPaths candidate = {0};
    int copied = snprintf(candidate.base_directory, sizeof(candidate.base_directory),
                          "%s", base);
    if (copied < 0 || (size_t)copied >= sizeof(candidate.base_directory))
        return fail(message, message_size, "Configuration base is too long", base);
    if (!join_path(candidate.directory, sizeof(candidate.directory), base, "mt") ||
        !join_path(candidate.keymap_path, sizeof(candidate.keymap_path),
                   candidate.directory, "keymap.conf") ||
        !join_path(candidate.settings_path, sizeof(candidate.settings_path),
                   candidate.directory, "settings.conf") ||
        !join_path(candidate.packages_path, sizeof(candidate.packages_path),
                   candidate.directory, "packages"))
        return fail(message, message_size, "Configuration path is too long", base);
    *paths = candidate;
    return true;
}

bool config_paths_discover(ConfigPaths *paths, char *message, size_t message_size)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg)
        return config_paths_from_base(paths, xdg, message, message_size);
    const char *home = getenv("HOME");
    if (!home || !*home)
        return fail(message, message_size, "HOME is not set", NULL);
    char base[MT_CONFIG_PATH_SIZE];
    if (!join_path(base, sizeof(base), home, ".config"))
        return fail(message, message_size, "HOME configuration path is too long", home);
    return config_paths_from_base(paths, base, message, message_size);
}

bool config_paths_prepare(const ConfigPaths *paths, char *message, size_t message_size)
{
    if (!paths || !paths->base_directory[0] || !paths->directory[0] ||
        !paths->keymap_path[0] || !paths->settings_path[0] || !paths->packages_path[0])
        return fail(message, message_size, "Configuration paths are incomplete", NULL);
    if (!ensure_directory(paths->base_directory, message, message_size) ||
        !ensure_directory(paths->directory, message, message_size) ||
        !ensure_directory(paths->packages_path, message, message_size))
        return false;

    FILE *file = fopen(paths->keymap_path, "r");
    if (file) {
        if (fclose(file) != 0)
            return fail(message, message_size, "Unable to close keymap",
                        paths->keymap_path);
    } else {
        if (errno != ENOENT)
            return fail(message, message_size, strerror(errno), paths->keymap_path);
        file = fopen(paths->keymap_path, "wx");
        if (!file && errno == EEXIST) {
            file = fopen(paths->keymap_path, "r");
            if (!file)
                return fail(message, message_size, strerror(errno), paths->keymap_path);
            if (fclose(file) != 0)
                return fail(message, message_size, "Unable to close keymap",
                            paths->keymap_path);
        } else if (!file)
            return fail(message, message_size, strerror(errno), paths->keymap_path);
        else if (fputs("# mt keymap: key = command\n"
                       "alt+x = execute-command\n"
                       "ctrl+o = find-file\n"
                       "ctrl+d = dired\n"
                       "ctrl+b = next-buffer\n",
                       file) == EOF ||
                 fclose(file) != 0)
            return fail(message, message_size, "Unable to write default keymap",
                        paths->keymap_path);
    }
    snprintf(message, message_size, "Configuration: %s", paths->directory);
    return true;
}
