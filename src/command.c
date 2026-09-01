#include "command.h"
#include <stddef.h>
#include <string.h>
typedef struct {
    Command command;
    const char *name;
} CommandName;
static const CommandName COMMANDS[] = {
    {COMMAND_SAVE, "save"},
    {COMMAND_SELECT_ALL, "select-all"},
    {COMMAND_COPY, "copy"},
    {COMMAND_CUT, "cut"},
    {COMMAND_PASTE, "paste"},
    {COMMAND_BACKSPACE, "backspace"},
    {COMMAND_DELETE, "delete"},
    {COMMAND_NEWLINE, "newline"},
    {COMMAND_TAB, "tab"},
    {COMMAND_CURSOR_LEFT, "cursor-left"},
    {COMMAND_CURSOR_RIGHT, "cursor-right"},
    {COMMAND_CURSOR_UP, "cursor-up"},
    {COMMAND_CURSOR_DOWN, "cursor-down"},
    {COMMAND_WORD_LEFT, "word-left"},
    {COMMAND_WORD_RIGHT, "word-right"},
    {COMMAND_LINE_START, "line-start"},
    {COMMAND_LINE_END, "line-end"},
    {COMMAND_BUFFER_START, "buffer-start"},
    {COMMAND_BUFFER_END, "buffer-end"},
    {COMMAND_KILL_LINE, "kill-line"},
    {COMMAND_ISEARCH, "isearch"},
    {COMMAND_QUERY_REPLACE, "query-replace"},
    {COMMAND_PAGE_UP, "page-up"},
    {COMMAND_PAGE_DOWN, "page-down"},
    {COMMAND_EXECUTE_COMMAND, "execute-command"},
    {COMMAND_SHELL, "cmd"},
    {COMMAND_FIND_FILE, "find-file"},
    {COMMAND_DIRED, "dired"},
    {COMMAND_NEXT_BUFFER, "next-buffer"},
    {COMMAND_DIRED_OPEN, "dired-open"},
    {COMMAND_DIRED_REFRESH, "dired-refresh"},
    {COMMAND_DIRED_CREATE_FILE, "dired-create-file"},
    {COMMAND_DIRED_CREATE_DIRECTORY, "dired-create-directory"},
    {COMMAND_DIRED_RENAME, "dired-rename"},
    {COMMAND_DIRED_DELETE, "dired-delete"},
    {COMMAND_LIST_COMMANDS, "list-commands"},
    {COMMAND_QUIT, "quit"}};
const char *command_name(Command command)
{
    for (size_t i = 0; i < sizeof(COMMANDS) / sizeof(*COMMANDS); i++)
        if (COMMANDS[i].command == command)
            return COMMANDS[i].name;
    return "none";
}
Command command_from_name(const char *name)
{
    for (size_t i = 0; i < sizeof(COMMANDS) / sizeof(*COMMANDS); i++)
        if (strcmp(COMMANDS[i].name, name) == 0)
            return COMMANDS[i].command;
    return COMMAND_NONE;
}

size_t command_count(void)
{
    return sizeof(COMMANDS) / sizeof(*COMMANDS);
}

const char *command_name_at(size_t index)
{
    return index < command_count() ? COMMANDS[index].name : NULL;
}
