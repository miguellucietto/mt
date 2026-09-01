#include "keymap.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Retains only modifiers that participate in configurable bindings. */
static SDL_Keymod normalized_modifiers(SDL_Keymod modifiers)
{
    SDL_Keymod normalized = SDL_KMOD_NONE;
    /* Os aliases CTRL/SHIFT/etc. contêm os bits esquerdo e direito. Eventos
       reais trazem apenas o lado pressionado; convertemos ambos para a mesma
       representação antes de comparar. */
    if (modifiers & SDL_KMOD_CTRL)
        normalized |= SDL_KMOD_CTRL;
    if (modifiers & SDL_KMOD_SHIFT)
        normalized |= SDL_KMOD_SHIFT;
    if (modifiers & SDL_KMOD_ALT)
        normalized |= SDL_KMOD_ALT;
    if (modifiers & SDL_KMOD_GUI)
        normalized |= SDL_KMOD_GUI;
    return normalized;
}

bool keymap_bind(Keymap *map, SDL_Keycode key, SDL_Keymod modifiers,
                 const char *command)
{
    modifiers = normalized_modifiers(modifiers);
    for (size_t i = 0; i < map->count; i++)
        if (map->bindings[i].key == key && map->bindings[i].modifiers == modifiers) {
            snprintf(map->bindings[i].command, sizeof(map->bindings[i].command), "%s",
                     command);
            return true;
        }
    if (map->count >= KEYMAP_MAX_BINDINGS)
        return false;
    KeyBinding *binding = &map->bindings[map->count++];
    binding->key = key;
    binding->modifiers = modifiers;
    snprintf(binding->command, sizeof(binding->command), "%s", command);
    return true;
}

void keymap_init_default(Keymap *map)
{
    map->count = 0;
    const KeyBinding defaults[] = {{SDLK_S, SDL_KMOD_CTRL, "save"},
                                   {SDLK_A, SDL_KMOD_CTRL, "select-all"},
                                   {SDLK_C, SDL_KMOD_CTRL, "copy"},
                                   {SDLK_X, SDL_KMOD_CTRL, "cut"},
                                   {SDLK_V, SDL_KMOD_CTRL, "paste"},
                                   {SDLK_Z, SDL_KMOD_CTRL, "undo"},
                                   {SDLK_Z, SDL_KMOD_CTRL | SDL_KMOD_SHIFT, "redo"},
                                   {SDLK_Q, SDL_KMOD_CTRL, "quit"},
                                   {SDLK_X, SDL_KMOD_ALT, "execute-command"},
                                   {SDLK_T, SDL_KMOD_ALT, "cmd"},
                                   {SDLK_O, SDL_KMOD_CTRL, "find-file"},
                                   {SDLK_D, SDL_KMOD_CTRL, "dired"},
                                   {SDLK_B, SDL_KMOD_CTRL, "next-buffer"},
                                   {SDLK_K, SDL_KMOD_CTRL, "kill-line"},
                                   {SDLK_F, SDL_KMOD_CTRL, "isearch"},
                                   {SDLK_BACKSPACE, SDL_KMOD_NONE, "backspace"},
                                   {SDLK_DELETE, SDL_KMOD_NONE, "delete"},
                                   {SDLK_RETURN, SDL_KMOD_NONE, "newline"},
                                   {SDLK_KP_ENTER, SDL_KMOD_NONE, "newline"},
                                   {SDLK_TAB, SDL_KMOD_NONE, "tab"},
                                   {SDLK_LEFT, SDL_KMOD_NONE, "cursor-left"},
                                   {SDLK_RIGHT, SDL_KMOD_NONE, "cursor-right"},
                                   {SDLK_LEFT, SDL_KMOD_CTRL, "word-left"},
                                   {SDLK_RIGHT, SDL_KMOD_CTRL, "word-right"},
                                   {SDLK_UP, SDL_KMOD_NONE, "cursor-up"},
                                   {SDLK_DOWN, SDL_KMOD_NONE, "cursor-down"},
                                   {SDLK_HOME, SDL_KMOD_NONE, "line-start"},
                                   {SDLK_END, SDL_KMOD_NONE, "line-end"},
                                   {SDLK_HOME, SDL_KMOD_CTRL, "buffer-start"},
                                   {SDLK_END, SDL_KMOD_CTRL, "buffer-end"},
                                   {SDLK_PAGEUP, SDL_KMOD_NONE, "page-up"},
                                   {SDLK_PAGEDOWN, SDL_KMOD_NONE, "page-down"}};
    for (size_t i = 0; i < SDL_arraysize(defaults); i++)
        keymap_bind(map, defaults[i].key, defaults[i].modifiers, defaults[i].command);
}

const char *keymap_lookup(const Keymap *map, const SDL_KeyboardEvent *event)
{
    SDL_Keymod modifiers = normalized_modifiers(event->mod);
    for (size_t i = 0; i < map->count; i++) {
        if (map->bindings[i].key == event->key &&
            map->bindings[i].modifiers == modifiers)
            return map->bindings[i].command;
    }
    /* Shift sem binding próprio estende a seleção nos comandos de movimento. */
    if (modifiers & SDL_KMOD_SHIFT) {
        modifiers &= ~SDL_KMOD_SHIFT;
        for (size_t i = 0; i < map->count; i++) {
            const char *command = map->bindings[i].command;
            bool movement = strncmp(command, "cursor-", 7) == 0 ||
                            strncmp(command, "word-", 5) == 0 ||
                            strcmp(command, "line-start") == 0 ||
                            strcmp(command, "line-end") == 0 ||
                            strncmp(command, "page-", 5) == 0;
            if (movement && map->bindings[i].key == event->key &&
                map->bindings[i].modifiers == modifiers)
                return command;
        }
    }
    return NULL;
}

/* Trims surrounding ASCII whitespace in place and returns the first content byte. */
static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        text++;
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return text;
}

/* Parses a mutable key specification into an SDL key and normalized modifiers. */
static bool parse_key(char *spec, SDL_Keycode *key, SDL_Keymod *modifiers)
{
    *modifiers = SDL_KMOD_NONE;
    *key = 0;
    char *token = strtok(spec, "+");
    while (token) {
        token = trim(token);
        if (strcmp(token, "ctrl") == 0)
            *modifiers |= SDL_KMOD_CTRL;
        else if (strcmp(token, "shift") == 0)
            *modifiers |= SDL_KMOD_SHIFT;
        else if (strcmp(token, "alt") == 0)
            *modifiers |= SDL_KMOD_ALT;
        else if (strcmp(token, "super") == 0)
            *modifiers |= SDL_KMOD_GUI;
        else {
            *key = SDL_GetKeyFromName(token);
            if (!*key)
                return false;
        }
        token = strtok(NULL, "+");
    }
    return *key != 0;
}

bool keymap_load(Keymap *map, const char *path, char *message, size_t message_size)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        snprintf(message, message_size, "Keymap não encontrado: %s", path);
        return false;
    }
    char line[256];
    int number = 0, loaded = 0;
    while (fgets(line, sizeof(line), file)) {
        number++;
        char *content = trim(line);
        if (!*content || *content == '#')
            continue;
        char *equals = strchr(content, '=');
        if (!equals) {
            snprintf(message, message_size, "Keymap: linha %d inválida", number);
            fclose(file);
            return false;
        }
        *equals = '\0';
        char *key_text = trim(content), *command_text = trim(equals + 1);
        SDL_Keycode key;
        SDL_Keymod modifiers;
        if (!*command_text || !parse_key(key_text, &key, &modifiers) ||
            !keymap_bind(map, key, modifiers, command_text)) {
            snprintf(message, message_size, "Keymap: erro na linha %d", number);
            fclose(file);
            return false;
        }
        loaded++;
    }
    fclose(file);
    snprintf(message, message_size, "Keymap: %d atalhos carregados", loaded);
    return true;
}
