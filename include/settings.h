#ifndef MT_SETTINGS_H
#define MT_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int window_width;
    int window_height;
    float font_size;
    int line_spacing;
    int gutter_width;
    int top_height;
    int status_height;
    int padding;
    int tab_width;
    bool tab_insert_spaces;
    bool search_wrap;
    bool search_case_sensitive;
    size_t process_output_limit;
} Settings;

/** Replaces settings with the complete built-in, behavior-preserving defaults. */
void settings_init_defaults(Settings *settings);
/** Validates every field and reports the first violated settings contract. */
bool settings_validate(const Settings *settings, char *message, size_t message_size);
/** Creates a documented default settings file only when path does not exist. */
bool settings_ensure_file(const char *path, char *message, size_t message_size);
/** Loads a complete settings candidate and commits it only after full validation. */
bool settings_load_file(Settings *settings, const char *path, char *message,
                        size_t message_size);

#endif
