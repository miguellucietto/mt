#include "editing.h"

#include "text.h"

#include <SDL3/SDL.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Copies selected bytes to the platform clipboard and optionally erases them. */
static void copy_selection(Editor *editor, bool erase)
{
    Document *document = editor_current_document(editor);
    size_t start = document_selection_start(document);
    size_t end = document_selection_end(document);
    if (start == end)
        return;
    char *text = malloc(end - start + 1);
    if (!text)
        return;
    memcpy(text, document->text + start, end - start);
    text[end - start] = '\0';
    SDL_SetClipboardText(text);
    free(text);
    if (erase)
        document_erase(document, start, end);
}

/* Moves by logical lines while preserving the editor's desired column. */
static void move_vertical(Editor *editor, int delta, bool selecting)
{
    Document *document = editor_current_document(editor);
    int line = text_line_at(document, document->cursor);
    if (editor->wanted_column < 0)
        editor->wanted_column = text_column_at(document, document->cursor);
    int target = line + delta;
    if (target >= 0)
        editor_set_cursor(editor,
                          text_position_at(document, target, editor->wanted_column),
                          selecting);
}

/* Classifies one byte position for the editor's ASCII-oriented word navigation. */
static bool word_character(const char *text, size_t position)
{
    unsigned char character = (unsigned char)text[position];
    return character >= 0x80 || isalnum(character) || character == '_';
}

/* Finds the previous word boundary without moving before the document. */
static size_t previous_word(const Document *document, size_t position)
{
    while (position && !word_character(document->text, text_previous_codepoint(
                                                           document->text, position)))
        position = text_previous_codepoint(document->text, position);
    while (position && word_character(document->text, text_previous_codepoint(
                                                          document->text, position)))
        position = text_previous_codepoint(document->text, position);
    return position;
}

/* Finds the next word boundary without moving beyond the document. */
static size_t next_word(const Document *document, size_t position)
{
    while (position < document->length && word_character(document->text, position))
        position = text_next_codepoint(document->text, document->length, position);
    while (position < document->length && !word_character(document->text, position))
        position = text_next_codepoint(document->text, document->length, position);
    return position;
}

/* Selects every byte in the active document. */
static void command_select_all(Editor *editor, bool selecting)
{
    (void)selecting;
    Document *document = editor_current_document(editor);
    document->anchor = 0;
    document->cursor = document->length;
}

/* Copies the active selection without changing the document. */
static void command_copy(Editor *editor, bool selecting)
{
    (void)selecting;
    copy_selection(editor, false);
}

/* Copies and removes the active selection when the buffer is writable. */
static void command_cut(Editor *editor, bool selecting)
{
    (void)selecting;
    if (!editor_current_buffer(editor)->read_only)
        copy_selection(editor, true);
}

/* Inserts clipboard text into the active writable document. */
static void command_paste(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    char *text = SDL_GetClipboardText();
    if (!buffer->read_only && text)
        document_insert(&buffer->document, text);
    SDL_free(text);
}

/* Reverses the latest active-document edit. */
static void command_undo(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only)
        document_undo(&buffer->document);
}

/* Reapplies the latest reverted active-document edit. */
static void command_redo(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only)
        document_redo(&buffer->document);
}

/* Removes the selection or preceding UTF-8 codepoint in a writable buffer. */
static void command_backspace(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    Document *document = &buffer->document;
    if (buffer->read_only)
        return;
    if (document_has_selection(document))
        document_erase(document, document_selection_start(document),
                       document_selection_end(document));
    else
        document_erase(document,
                       text_previous_codepoint(document->text, document->cursor),
                       document->cursor);
}

/* Removes the selection or following UTF-8 codepoint in a writable buffer. */
static void command_delete(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    Document *document = &buffer->document;
    if (buffer->read_only)
        return;
    if (document_has_selection(document))
        document_erase(document, document_selection_start(document),
                       document_selection_end(document));
    else
        document_erase(
            document, document->cursor,
            text_next_codepoint(document->text, document->length, document->cursor));
}

/* Inserts the configured tab representation into a writable buffer. */
static void command_tab(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (buffer->read_only)
        return;
    if (!editor->settings.tab_insert_spaces) {
        document_insert(&buffer->document, "\t");
        return;
    }
    char spaces[17];
    int width = editor->settings.tab_width;
    if (width < 1)
        width = 1;
    if (width > (int)sizeof(spaces) - 1)
        width = (int)sizeof(spaces) - 1;
    memset(spaces, ' ', (size_t)width);
    spaces[width] = '\0';
    document_insert(&buffer->document, spaces);
}

/* Moves or extends the cursor one UTF-8 codepoint to the left. */
static void command_cursor_left(Editor *editor, bool selecting)
{
    Document *document = editor_current_document(editor);
    editor_set_cursor(editor, text_previous_codepoint(document->text, document->cursor),
                      selecting);
}

/* Moves or extends the cursor one UTF-8 codepoint to the right. */
static void command_cursor_right(Editor *editor, bool selecting)
{
    Document *document = editor_current_document(editor);
    editor_set_cursor(
        editor, text_next_codepoint(document->text, document->length, document->cursor),
        selecting);
}

/* Moves or extends the cursor one logical line upward. */
static void command_cursor_up(Editor *editor, bool selecting)
{
    move_vertical(editor, -1, selecting);
}

/* Moves or extends the cursor one logical line downward. */
static void command_cursor_down(Editor *editor, bool selecting)
{
    move_vertical(editor, 1, selecting);
}

/* Moves or extends the cursor to the previous word boundary. */
static void command_word_left(Editor *editor, bool selecting)
{
    Document *document = editor_current_document(editor);
    editor_set_cursor(editor, previous_word(document, document->cursor), selecting);
}

/* Moves or extends the cursor to the next word boundary. */
static void command_word_right(Editor *editor, bool selecting)
{
    Document *document = editor_current_document(editor);
    editor_set_cursor(editor, next_word(document, document->cursor), selecting);
}

/* Moves or extends the cursor to the current logical line start. */
static void command_line_start(Editor *editor, bool selecting)
{
    Document *document = editor_current_document(editor);
    editor_set_cursor(editor, text_line_start(document, document->cursor), selecting);
}

/* Moves or extends the cursor to the current logical line end. */
static void command_line_end(Editor *editor, bool selecting)
{
    Document *document = editor_current_document(editor);
    editor_set_cursor(editor, text_line_end(document, document->cursor), selecting);
}

/* Moves or extends the cursor to the start of the document. */
static void command_buffer_start(Editor *editor, bool selecting)
{
    editor_set_cursor(editor, 0, selecting);
}

/* Moves or extends the cursor to the end of the document. */
static void command_buffer_end(Editor *editor, bool selecting)
{
    editor_set_cursor(editor, editor_current_document(editor)->length, selecting);
}

/* Deletes through line end, or the newline when already at line end. */
static void command_kill_line(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    Document *document = &buffer->document;
    if (buffer->read_only)
        return;
    size_t end = text_line_end(document, document->cursor);
    if (end == document->cursor && end < document->length)
        end++;
    document_erase(document, document->cursor, end);
}

/* Moves upward by the current viewport's logical line count. */
static void command_page_up(Editor *editor, bool selecting)
{
    move_vertical(editor, -10, selecting);
}

/* Moves downward by the current viewport's logical line count. */
static void command_page_down(Editor *editor, bool selecting)
{
    move_vertical(editor, 10, selecting);
}

typedef struct {
    const char *name;
    const char *description;
    unsigned int flags;
    CommandFunction function;
} EditingCommand;

bool editing_register_commands(Editor *editor)
{
    static const EditingCommand commands[] = {
        {"select-all", "Seleciona todo o buffer", 0, command_select_all},
        {"copy", "Copia a seleção", 0, command_copy},
        {"cut", "Recorta a seleção", 0, command_cut},
        {"paste", "Insere o conteúdo da área de transferência", 0, command_paste},
        {"undo", "Desfaz a última edição", 0, command_undo},
        {"redo", "Refaz a última edição desfeita", 0, command_redo},
        {"backspace", "Apaga antes do cursor", 0, command_backspace},
        {"delete", "Apaga depois do cursor", 0, command_delete},
        {"tab", "Insere indentação", 0, command_tab},
        {"cursor-left", "Move o cursor para a esquerda", 0, command_cursor_left},
        {"cursor-right", "Move o cursor para a direita", 0, command_cursor_right},
        {"cursor-up", "Move o cursor para cima", COMMAND_FLAG_KEEP_COLUMN,
         command_cursor_up},
        {"cursor-down", "Move o cursor para baixo", COMMAND_FLAG_KEEP_COLUMN,
         command_cursor_down},
        {"word-left", "Move para a palavra anterior", 0, command_word_left},
        {"word-right", "Move para a próxima palavra", 0, command_word_right},
        {"line-start", "Move para o início da linha", 0, command_line_start},
        {"line-end", "Move para o fim da linha", 0, command_line_end},
        {"buffer-start", "Move para o início do buffer", 0, command_buffer_start},
        {"buffer-end", "Move para o fim do buffer", 0, command_buffer_end},
        {"kill-line", "Apaga até o fim da linha", 0, command_kill_line},
        {"page-up", "Move uma página para cima", COMMAND_FLAG_KEEP_COLUMN,
         command_page_up},
        {"page-down", "Move uma página para baixo", COMMAND_FLAG_KEEP_COLUMN,
         command_page_down},
    };
    for (size_t i = 0; i < SDL_arraysize(commands); i++)
        if (!command_registry_register(&editor->commands, commands[i].name,
                                       commands[i].description, commands[i].flags,
                                       commands[i].function))
            return false;
    return true;
}
