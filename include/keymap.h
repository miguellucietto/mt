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
void keymap_init_default(Keymap *keymap);
bool keymap_bind(Keymap *keymap, SDL_Keycode key, SDL_Keymod modifiers,
                 const char *command);
const char *keymap_lookup(const Keymap *keymap, const SDL_KeyboardEvent *event);
bool keymap_load(Keymap *keymap, const char *path, char *message, size_t message_size);
#endif
