#ifndef MT_CONFIG_H
#define MT_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char directory[4096];
    char keymap_path[4096];
    char packages_path[4096];
} Config;

bool config_init(Config *config, char *message, size_t message_size);

#endif
