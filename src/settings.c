#include "settings.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SETTING_WINDOW_WIDTH,
    SETTING_WINDOW_HEIGHT,
    SETTING_FONT_SIZE,
    SETTING_LINE_SPACING,
    SETTING_GUTTER_WIDTH,
    SETTING_TOP_HEIGHT,
    SETTING_STATUS_HEIGHT,
    SETTING_PADDING,
    SETTING_TAB_WIDTH,
    SETTING_TAB_INSERT_SPACES,
    SETTING_SEARCH_WRAP,
    SETTING_SEARCH_CASE_SENSITIVE,
    SETTING_PROCESS_OUTPUT_LIMIT,
    SETTING_COUNT
} SettingId;

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

/* Formats a settings-file diagnostic with an exact source line. */
static bool file_error(char *message, size_t message_size, const char *path,
                       size_t line, const char *reason)
{
    if (message && message_size)
        snprintf(message, message_size, "%s:%zu: %s", path, line, reason);
    return false;
}

/* Resolves a stable configuration key to its typed settings field identifier. */
static bool setting_id(const char *name, SettingId *id)
{
    static const char *names[SETTING_COUNT] = {
        [SETTING_WINDOW_WIDTH] = "window.width",
        [SETTING_WINDOW_HEIGHT] = "window.height",
        [SETTING_FONT_SIZE] = "font.size",
        [SETTING_LINE_SPACING] = "layout.line_spacing",
        [SETTING_GUTTER_WIDTH] = "layout.gutter_width",
        [SETTING_TOP_HEIGHT] = "layout.top_height",
        [SETTING_STATUS_HEIGHT] = "layout.status_height",
        [SETTING_PADDING] = "layout.padding",
        [SETTING_TAB_WIDTH] = "tab.width",
        [SETTING_TAB_INSERT_SPACES] = "tab.insert_spaces",
        [SETTING_SEARCH_WRAP] = "search.wrap",
        [SETTING_SEARCH_CASE_SENSITIVE] = "search.case_sensitive",
        [SETTING_PROCESS_OUTPUT_LIMIT] = "process.output_limit",
    };
    for (size_t i = 0; i < SETTING_COUNT; i++) {
        if (strcmp(name, names[i]) == 0) {
            *id = (SettingId)i;
            return true;
        }
    }
    return false;
}

/* Parses a strict base-10 integer inside an inclusive range. */
static bool parse_int(const char *text, int minimum, int maximum, int *value)
{
    errno = 0;
    char *end;
    long parsed = strtol(text, &end, 10);
    if (errno || end == text || *trim(end) || parsed < minimum || parsed > maximum)
        return false;
    *value = (int)parsed;
    return true;
}

/* Parses a finite decimal float inside an inclusive range. */
static bool parse_float(const char *text, float minimum, float maximum, float *value)
{
    errno = 0;
    char *end;
    float parsed = strtof(text, &end);
    if (errno || end == text || *trim(end) || !isfinite(parsed) || parsed < minimum ||
        parsed > maximum)
        return false;
    *value = parsed;
    return true;
}

/* Parses the canonical true and false setting values. */
static bool parse_bool(const char *text, bool *value)
{
    if (strcmp(text, "true") == 0)
        *value = true;
    else if (strcmp(text, "false") == 0)
        *value = false;
    else
        return false;
    return true;
}

/* Parses a bounded byte count without accepting signs or suffixes. */
static bool parse_size(const char *text, size_t minimum, size_t maximum, size_t *value)
{
    if (*text == '-' || *text == '+')
        return false;
    errno = 0;
    char *end;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno || end == text || *trim(end) || parsed < minimum || parsed > maximum ||
        parsed > SIZE_MAX)
        return false;
    *value = (size_t)parsed;
    return true;
}

void settings_init_defaults(Settings *settings)
{
    *settings = (Settings){
        .window_width = 1000,
        .window_height = 700,
        .font_size = 18.0f,
        .line_spacing = 4,
        .gutter_width = 58,
        .top_height = 40,
        .status_height = 27,
        .padding = 10,
        .tab_width = 4,
        .tab_insert_spaces = true,
        .search_wrap = true,
        .search_case_sensitive = true,
        .process_output_limit = 16 * 1024 * 1024,
    };
}

bool settings_validate(const Settings *settings, char *message, size_t message_size)
{
    if (!settings) {
        if (message && message_size)
            snprintf(message, message_size, "Settings are missing");
        return false;
    }
    if (!isfinite(settings->font_size)) {
        if (message && message_size)
            snprintf(message, message_size, "font_size is not finite");
        return false;
    }
#define CHECK_RANGE(field, minimum, maximum)                                           \
    if (settings->field < (minimum) || settings->field > (maximum)) {                  \
        if (message && message_size)                                                   \
            snprintf(message, message_size, #field " is outside its valid range");     \
        return false;                                                                  \
    }
    CHECK_RANGE(window_width, 640, 7680)
    CHECK_RANGE(window_height, 480, 4320)
    CHECK_RANGE(font_size, 8.0f, 96.0f)
    CHECK_RANGE(line_spacing, 0, 32)
    CHECK_RANGE(gutter_width, 0, 256)
    CHECK_RANGE(top_height, 16, 128)
    CHECK_RANGE(status_height, 16, 128)
    CHECK_RANGE(padding, 0, 64)
    CHECK_RANGE(tab_width, 1, 16)
    CHECK_RANGE(process_output_limit, 1024, (size_t)1024 * 1024 * 1024)
#undef CHECK_RANGE
    return true;
}

bool settings_ensure_file(const char *path, char *message, size_t message_size)
{
    if (!path || !*path)
        return file_error(message, message_size, path ? path : "<settings>", 0,
                          "invalid settings path");
    FILE *file = fopen(path, "r");
    if (file) {
        if (fclose(file) != 0)
            return file_error(message, message_size, path, 0,
                              "unable to close settings");
        return true;
    }
    if (errno != ENOENT)
        return file_error(message, message_size, path, 0, strerror(errno));
    file = fopen(path, "wx");
    if (!file && errno == EEXIST) {
        file = fopen(path, "r");
        if (!file)
            return file_error(message, message_size, path, 0, strerror(errno));
        if (fclose(file) != 0)
            return file_error(message, message_size, path, 0,
                              "unable to close settings");
        return true;
    }
    if (!file)
        return file_error(message, message_size, path, 0, strerror(errno));
    static const char contents[] = "# mt settings: key = value\n"
                                   "window.width = 1000\n"
                                   "window.height = 700\n"
                                   "font.size = 18\n"
                                   "layout.line_spacing = 4\n"
                                   "layout.gutter_width = 58\n"
                                   "layout.top_height = 40\n"
                                   "layout.status_height = 27\n"
                                   "layout.padding = 10\n"
                                   "tab.width = 4\n"
                                   "tab.insert_spaces = true\n"
                                   "search.wrap = true\n"
                                   "search.case_sensitive = true\n"
                                   "process.output_limit = 16777216\n";
    bool written =
        fwrite(contents, 1, sizeof(contents) - 1, file) == sizeof(contents) - 1;
    if (fclose(file) != 0)
        written = false;
    if (!written)
        return file_error(message, message_size, path, 0,
                          "unable to write default settings");
    return true;
}

/* Parses and applies one already-separated key/value pair to a candidate. */
static bool apply_setting(Settings *candidate, SettingId id, const char *value)
{
    switch (id) {
    case SETTING_WINDOW_WIDTH:
        return parse_int(value, 640, 7680, &candidate->window_width);
    case SETTING_WINDOW_HEIGHT:
        return parse_int(value, 480, 4320, &candidate->window_height);
    case SETTING_FONT_SIZE:
        return parse_float(value, 8.0f, 96.0f, &candidate->font_size);
    case SETTING_LINE_SPACING:
        return parse_int(value, 0, 32, &candidate->line_spacing);
    case SETTING_GUTTER_WIDTH:
        return parse_int(value, 0, 256, &candidate->gutter_width);
    case SETTING_TOP_HEIGHT:
        return parse_int(value, 16, 128, &candidate->top_height);
    case SETTING_STATUS_HEIGHT:
        return parse_int(value, 16, 128, &candidate->status_height);
    case SETTING_PADDING:
        return parse_int(value, 0, 64, &candidate->padding);
    case SETTING_TAB_WIDTH:
        return parse_int(value, 1, 16, &candidate->tab_width);
    case SETTING_TAB_INSERT_SPACES:
        return parse_bool(value, &candidate->tab_insert_spaces);
    case SETTING_SEARCH_WRAP:
        return parse_bool(value, &candidate->search_wrap);
    case SETTING_SEARCH_CASE_SENSITIVE:
        return parse_bool(value, &candidate->search_case_sensitive);
    case SETTING_PROCESS_OUTPUT_LIMIT:
        return parse_size(value, 1024, (size_t)1024 * 1024 * 1024,
                          &candidate->process_output_limit);
    case SETTING_COUNT:
        return false;
    }
    return false;
}

bool settings_load_file(Settings *settings, const char *path, char *message,
                        size_t message_size)
{
    if (!settings || !path || !*path)
        return file_error(message, message_size, path ? path : "<settings>", 0,
                          "invalid settings arguments");
    FILE *file = fopen(path, "r");
    if (!file) {
        if (errno == ENOENT) {
            if (message && message_size)
                snprintf(message, message_size, "Settings: defaults (%s not found)",
                         path);
            return true;
        }
        return file_error(message, message_size, path, 0, strerror(errno));
    }

    Settings candidate = *settings;
    uint32_t seen = 0;
    char line[512];
    size_t line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (!strchr(line, '\n') && !feof(file)) {
            fclose(file);
            return file_error(message, message_size, path, line_number,
                              "line exceeds 511 bytes");
        }
        char *content = trim(line);
        if (!*content || *content == '#')
            continue;
        char *equals = strchr(content, '=');
        if (!equals) {
            fclose(file);
            return file_error(message, message_size, path, line_number,
                              "expected key = value");
        }
        *equals = '\0';
        char *name = trim(content);
        char *value = trim(equals + 1);
        SettingId id;
        if (!setting_id(name, &id)) {
            fclose(file);
            return file_error(message, message_size, path, line_number,
                              "unknown setting");
        }
        uint32_t bit = UINT32_C(1) << id;
        if (seen & bit) {
            fclose(file);
            return file_error(message, message_size, path, line_number,
                              "duplicate setting");
        }
        if (!*value || !apply_setting(&candidate, id, value)) {
            fclose(file);
            return file_error(message, message_size, path, line_number,
                              "invalid setting value");
        }
        seen |= bit;
    }
    if (ferror(file)) {
        fclose(file);
        return file_error(message, message_size, path, line_number,
                          "unable to read settings");
    }
    if (fclose(file) != 0)
        return file_error(message, message_size, path, line_number,
                          "unable to close settings");
    char validation[128];
    if (!settings_validate(&candidate, validation, sizeof(validation)))
        return file_error(message, message_size, path, line_number, validation);
    *settings = candidate;
    if (message && message_size)
        snprintf(message, message_size, "Settings loaded: %s", path);
    return true;
}
