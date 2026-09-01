#ifndef MT_KEYMAP_H
#define MT_KEYMAP_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#define KEYMAP_MAX_BINDINGS 64
typedef struct {
    SDL_Keycode key;
    SDL_Keymod modifiers;
    char command[64];
} KeyBinding;
typedef struct {
    KeyBinding bindings[KEYMAP_MAX_BINDINGS];
    size_t count;
} Keymap;
/** Replaces a keymap with the built-in default bindings. */
void keymap_init_default(Keymap *keymap);
/** Adds or replaces one normalized key binding when capacity permits. */
bool keymap_bind(Keymap *keymap, SDL_Keycode key, SDL_Keymod modifiers,
                 const char *command);
/** Resolves a keyboard event to a borrowed command name, or NULL. */
const char *keymap_lookup(const Keymap *keymap, const SDL_KeyboardEvent *event);
/** Parses bindings from path into the keymap and reports load errors. */
bool keymap_load(Keymap *keymap, const char *path, char *message, size_t message_size);
#endif
