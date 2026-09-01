#include "search.h"

#include "editor.h"

#include <stdio.h>
#include <string.h>

static bool find_text(Editor *editor, size_t start, bool wrap)
{
    Document *document = editor_current_document(editor);
    SearchState *search = &editor->search;
    if (!search->query[0]) {
        document->cursor = document->anchor = search->origin;
        return false;
    }
    const char *match = strstr(document->text + start, search->query);
    if (!match && wrap && start > 0) {
        match = strstr(document->text, search->query);
        if (match && (size_t)(match - document->text) >= start)
            match = NULL;
    }
    if (!match)
        return false;
    document->anchor = (size_t)(match - document->text);
    document->cursor = document->anchor + strlen(search->query);
    editor_ensure_cursor_visible(editor);
    return true;
}

void search_update(Editor *editor)
{
    SearchState *search = &editor->search;
    snprintf(search->query, sizeof(search->query), "%s", editor->minibuffer.input);
    if (!find_text(editor, search->origin, true) && search->query[0])
        snprintf(editor->message, sizeof(editor->message), "Sem resultado: %.180s",
                 search->query);
}

void search_next(Editor *editor)
{
    Document *document = editor_current_document(editor);
    find_text(editor, document_selection_end(document), true);
}

void search_cancel(Editor *editor)
{
    Document *document = editor_current_document(editor);
    document->cursor = document->anchor = editor->search.origin;
}

static bool query_next(Editor *editor)
{
    Document *document = editor_current_document(editor);
    if (!find_text(editor, document_selection_end(document), false)) {
        minibuffer_close(&editor->minibuffer);
        snprintf(editor->message, sizeof(editor->message), "Query replace concluído");
        return false;
    }
    minibuffer_open(&editor->minibuffer, MINIBUFFER_QUERY_CONFIRM,
                    "Replace? y/n/!/q: ");
    return true;
}

static void replace_current_match(Editor *editor)
{
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only)
        document_insert(&buffer->document, editor->search.replacement);
}

bool search_handle_confirmation(Editor *editor, const SDL_Event *event)
{
    if (editor->minibuffer.mode != MINIBUFFER_QUERY_CONFIRM)
        return false;
    if (event->type != SDL_EVENT_KEY_DOWN)
        return true;
    SDL_Keycode key = event->key.key;
    if (key == SDLK_Y) {
        replace_current_match(editor);
        query_next(editor);
    } else if (key == SDLK_N) {
        Document *document = editor_current_document(editor);
        document->cursor = document->anchor = document_selection_end(document);
        query_next(editor);
    } else if (key == SDLK_EXCLAIM ||
               (key == SDLK_1 && (event->key.mod & SDL_KMOD_SHIFT))) {
        do {
            replace_current_match(editor);
        } while (query_next(editor));
    } else if (key == SDLK_Q || key == SDLK_ESCAPE) {
        minibuffer_close(&editor->minibuffer);
        snprintf(editor->message, sizeof(editor->message), "Query replace encerrado");
    }
    return true;
}

bool search_submit(Editor *editor, MinibufferMode mode, const char *value)
{
    SearchState *search = &editor->search;
    if (mode == MINIBUFFER_ISEARCH) {
        snprintf(search->query, sizeof(search->query), "%s", value);
    } else if (mode == MINIBUFFER_QUERY_FIND) {
        snprintf(search->query, sizeof(search->query), "%s", value);
        minibuffer_open(&editor->minibuffer, MINIBUFFER_QUERY_REPLACE,
                        "Replace with: ");
    } else if (mode == MINIBUFFER_QUERY_REPLACE) {
        snprintf(search->replacement, sizeof(search->replacement), "%s", value);
        Document *document = editor_current_document(editor);
        document->cursor = document->anchor = search->origin;
        query_next(editor);
    } else
        return false;
    return true;
}

static void command_isearch(Editor *editor, bool selecting)
{
    (void)selecting;
    editor->search.origin = editor_current_document(editor)->cursor;
    editor->search.query[0] = '\0';
    minibuffer_open(&editor->minibuffer, MINIBUFFER_ISEARCH, "I-search: ");
}

static void command_query_replace(Editor *editor, bool selecting)
{
    (void)selecting;
    Buffer *buffer = editor_current_buffer(editor);
    if (!buffer->read_only) {
        editor->search.origin = buffer->document.cursor;
        minibuffer_open(&editor->minibuffer, MINIBUFFER_QUERY_FIND, "Query replace: ");
    }
}

bool search_register_commands(Editor *editor)
{
    return command_registry_register(&editor->commands, "isearch",
                                     "Inicia busca incremental",
                                     COMMAND_FLAG_OPENS_MINIBUFFER, command_isearch) &&
           command_registry_register(&editor->commands, "query-replace",
                                     "Substitui ocorrências com confirmação",
                                     COMMAND_FLAG_OPENS_MINIBUFFER,
                                     command_query_replace);
}
