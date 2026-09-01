#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Ensures path names an accessible directory, creating it when absent. */
static bool ensure_directory(const char *path)
{
    return mkdir(path, 0755) == 0 || errno == EEXIST;
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

bool config_init(Config *config, char *message, size_t message_size)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    char base[4096];
    if (xdg && *xdg)
        snprintf(base, sizeof(base), "%s", xdg);
    else {
        const char *home = getenv("HOME");
        if (!home || !*home)
            return false;
        snprintf(base, sizeof(base), "%s/.config", home);
    }
    if (!ensure_directory(base))
        return false;
    if (!join_path(config->directory, sizeof(config->directory), base, "mt"))
        return false;
    if (!ensure_directory(config->directory))
        return false;
    if (!join_path(config->keymap_path, sizeof(config->keymap_path), config->directory,
                   "keymap.conf") ||
        !join_path(config->packages_path, sizeof(config->packages_path),
                   config->directory, "packages"))
        return false;
    if (!ensure_directory(config->packages_path))
        return false;
    FILE *file = fopen(config->keymap_path, "r");
    if (file)
        fclose(file);
    else {
        file = fopen(config->keymap_path, "w");
        if (!file)
            return false;
        fputs("# keymap do mt: tecla = comando\n"
              "alt+x = execute-command\n"
              "ctrl+o = find-file\n"
              "ctrl+d = dired\n"
              "ctrl+b = next-buffer\n",
              file);
        fclose(file);
    }
    snprintf(message, message_size, "Configuração: %s", config->directory);
    return true;
}
