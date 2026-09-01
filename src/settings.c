#include "settings.h"

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
