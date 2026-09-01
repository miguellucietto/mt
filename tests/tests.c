#include "buffer.h"
#include "command.h"
#include "dired_controller.h"
#include "document.h"
#include "editing.h"
#include "highlight.h"
#include "keymap.h"
#include "process.h"
#include "settings.h"
#include "shell_controller.h"
#include "text.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int command_calls;

/* Records registry callback invocation without requiring a concrete editor. */
static void test_command_callback(struct Editor *editor, bool selecting)
{
    assert(!editor);
    command_calls += selecting ? 2 : 1;
}

/* Verifies command validation, lookup, execution, and fixed-capacity behavior. */
static void test_command_registry(void)
{
    CommandRegistry registry;
    command_registry_init(&registry);
    assert(command_registry_register(&registry, "test-command", "Comando de teste",
                                     COMMAND_FLAG_OPENS_MINIBUFFER,
                                     test_command_callback));
    assert(!command_registry_register(&registry, "test-command", "Duplicado", 0,
                                      test_command_callback));
    assert(!command_registry_register(&registry, "", "Sem nome", 0,
                                      test_command_callback));
    assert(!command_registry_register(&registry, "invalid", "Sem função", 0, NULL));
    char long_name[MT_COMMAND_NAME_SIZE + 1];
    memset(long_name, 'x', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    assert(!command_registry_register(&registry, long_name, "Nome longo", 0,
                                      test_command_callback));
    const CommandSpec *command = command_registry_find(&registry, "test-command");
    assert(command);
    assert(strcmp(command->description, "Comando de teste") == 0);
    assert(command->flags & COMMAND_FLAG_OPENS_MINIBUFFER);
    assert(command_registry_at(&registry, 0) == command);
    assert(!command_registry_at(&registry, 1));
    command_calls = 0;
    assert(command_registry_execute(&registry, "test-command", NULL, true));
    assert(command_calls == 2);
    assert(!command_registry_execute(&registry, "missing", NULL, false));

    char name[MT_COMMAND_NAME_SIZE];
    while (registry.count < MT_MAX_COMMANDS) {
        snprintf(name, sizeof(name), "command-%zu", registry.count);
        assert(command_registry_register(&registry, name, "Preenche o registro", 0,
                                         test_command_callback));
    }
    assert(!command_registry_register(&registry, "overflow", "Sem espaço", 0,
                                      test_command_callback));
}

/* Verifies editing registration and representative selection, movement, and undo. */
static void test_editing_controller(void)
{
    Editor editor = {0};
    settings_init_defaults(&editor.settings);
    command_registry_init(&editor.commands);
    assert(editing_register_commands(&editor));
    assert(editor.commands.count == 22);
    assert(command_registry_find(&editor.commands, "undo"));
    const CommandSpec *vertical = command_registry_find(&editor.commands, "cursor-up");
    assert(vertical && (vertical->flags & COMMAND_FLAG_KEEP_COLUMN));

    assert(buffers_init(&editor.buffers));
    Document *document = editor_current_document(&editor);
    assert(document_insert(document, "aç\nline"));
    assert(command_registry_execute(&editor.commands, "select-all", &editor, false));
    assert(document->anchor == 0 && document->cursor == document->length);
    assert(command_registry_execute(&editor.commands, "cursor-left", &editor, false));
    assert(document->cursor == strlen("aç\nlin"));
    assert(command_registry_execute(&editor.commands, "undo", &editor, false));
    assert(strcmp(document->text, "") == 0);
    assert(command_registry_execute(&editor.commands, "tab", &editor, false));
    assert(strcmp(document->text, "    ") == 0);
    assert(command_registry_execute(&editor.commands, "undo", &editor, false));
    editor.settings.tab_width = 2;
    assert(command_registry_execute(&editor.commands, "tab", &editor, false));
    assert(strcmp(document->text, "  ") == 0);
    assert(command_registry_execute(&editor.commands, "undo", &editor, false));
    editor.settings.tab_insert_spaces = false;
    assert(command_registry_execute(&editor.commands, "tab", &editor, false));
    assert(strcmp(document->text, "\t") == 0);
    buffers_destroy(&editor.buffers);
}

/* Verifies all behavior-preserving settings defaults from one initialization API. */
static void test_settings_defaults(void)
{
    Settings settings = {0};
    settings_init_defaults(&settings);
    assert(settings.window_width == 1000 && settings.window_height == 700);
    assert(settings.font_size == 18.0f && settings.line_spacing == 4);
    assert(settings.gutter_width == 58 && settings.top_height == 40);
    assert(settings.status_height == 27 && settings.padding == 10);
    assert(settings.tab_width == 4 && settings.tab_insert_spaces);
    assert(settings.search_wrap && settings.search_case_sensitive);
    assert(settings.process_output_limit == 16 * 1024 * 1024);
}

/* Verifies file command registration and protected quit confirmation flows. */
static void test_file_controller(void)
{
    Editor editor = {.running = true};
    command_registry_init(&editor.commands);
    assert(file_register_commands(&editor));
    assert(editor.commands.count == 3);
    const CommandSpec *find = command_registry_find(&editor.commands, "find-file");
    assert(find && (find->flags & COMMAND_FLAG_OPENS_MINIBUFFER));
    assert(buffers_init(&editor.buffers));
    assert(document_insert(editor_current_document(&editor), "unsaved"));

    file_request_quit(&editor);
    assert(editor.running);
    assert(editor.minibuffer.mode == MINIBUFFER_QUIT_CONFIRM);
    assert(file_submit(&editor, MINIBUFFER_QUIT_CONFIRM, "no"));
    assert(editor.running);
    assert(strcmp(editor.message, "Saída cancelada") == 0);
    assert(file_submit(&editor, MINIBUFFER_QUIT_CONFIRM, "yes"));
    assert(!editor.running);
    assert(!file_submit(&editor, MINIBUFFER_COMMAND, "yes"));
    buffers_destroy(&editor.buffers);
}

/* Verifies Dired registration, filesystem mutations, confirmation, and events. */
static void test_dired_controller(void)
{
    Editor editor = {0};
    command_registry_init(&editor.commands);
    assert(dired_register_commands(&editor));
    assert(editor.commands.count == 7);
    assert(command_registry_find(&editor.commands, "dired-delete"));
    assert(buffers_init(&editor.buffers));

    char root[] = "/tmp/mt-dired-test-XXXXXX";
    assert(mkdtemp(root));
    assert(dired_submit(&editor, MINIBUFFER_DIRED, root));
    Buffer *buffer = editor_current_buffer(&editor);
    assert(buffer && buffer->type == BUFFER_DIRECTORY);
    assert(strcmp(buffer->directory, root) == 0);

    assert(dired_submit(&editor, MINIBUFFER_CREATE_FILE, "created.txt"));
    char file_path[512];
    assert(snprintf(file_path, sizeof(file_path), "%s/created.txt", root) > 0);
    struct stat information;
    assert(stat(file_path, &information) == 0 && S_ISREG(information.st_mode));

    assert(dired_submit(&editor, MINIBUFFER_CREATE_DIRECTORY, "created-dir"));
    char directory_path[512];
    assert(snprintf(directory_path, sizeof(directory_path), "%s/created-dir", root) >
           0);
    assert(stat(directory_path, &information) == 0 && S_ISDIR(information.st_mode));

    snprintf(editor.dired.pending_path, sizeof(editor.dired.pending_path), "%s",
             file_path);
    assert(dired_submit(&editor, MINIBUFFER_DELETE_CONFIRM, "no"));
    assert(stat(file_path, &information) == 0);
    assert(dired_submit(&editor, MINIBUFFER_DELETE_CONFIRM, "yes"));
    assert(stat(file_path, &information) != 0);

    SDL_Event refresh = {0};
    refresh.type = SDL_EVENT_KEY_DOWN;
    refresh.key.key = SDLK_G;
    assert(dired_handle_event(&editor, &refresh));
    refresh.key.key = SDLK_A;
    assert(!dired_handle_event(&editor, &refresh));
    assert(!dired_submit(&editor, MINIBUFFER_COMMAND, "ignored"));

    buffers_destroy(&editor.buffers);
    assert(rmdir(directory_path) == 0);
    assert(rmdir(root) == 0);
}

/* Verifies owned output, combined streams, status, cleanup, and invalid arguments. */
static void test_process_execution(void)
{
    ProcessResult result;
    char error[256] = {0};
    assert(process_run_shell("printf stdout; printf stderr >&2", &result, error,
                             sizeof(error)));
    assert(strcmp(result.output, "stdoutstderr") == 0);
    assert(result.status == 0);
    process_result_destroy(&result);
    assert(!result.output && result.status == 0);
    assert(!process_run_shell(NULL, &result, error, sizeof(error)));
    assert(error[0]);
}

/* Verifies shell command registration and read-only output presentation. */
static void test_shell_controller(void)
{
    Editor editor = {0};
    command_registry_init(&editor.commands);
    assert(shell_register_commands(&editor));
    assert(editor.commands.count == 1);
    const CommandSpec *command = command_registry_find(&editor.commands, "cmd");
    assert(command && (command->flags & COMMAND_FLAG_OPENS_MINIBUFFER));
    assert(buffers_init(&editor.buffers));

    assert(shell_submit(&editor, MINIBUFFER_SHELL, "printf hello"));
    Buffer *buffer = editor_current_buffer(&editor);
    assert(buffer && buffer->type == BUFFER_SHELL);
    assert(strcmp(buffer->document.text, "$ printf hello\n\nhello") == 0);
    assert(buffer->read_only);
    assert(!buffer->document.dirty);
    assert(strcmp(editor.message, "Comando finalizado (0)") == 0);
    assert(!shell_submit(&editor, MINIBUFFER_COMMAND, "ignored"));
    buffers_destroy(&editor.buffers);
}

/* Verifies incremental search cancellation and interactive replacement state. */
static void test_search_controller(void)
{
    Editor editor = {0};
    command_registry_init(&editor.commands);
    assert(search_register_commands(&editor));
    assert(editor.commands.count == 2);
    assert(buffers_init(&editor.buffers));
    Document *document = editor_current_document(&editor);
    assert(document_insert(document, "one two one"));
    document->cursor = document->anchor = 0;

    assert(command_registry_execute(&editor.commands, "isearch", &editor, false));
    minibuffer_insert(&editor.minibuffer, "two");
    search_update(&editor);
    assert(document_selection_start(document) == 4);
    assert(document_selection_end(document) == 7);
    search_cancel(&editor);
    assert(document->cursor == 0 && document->anchor == 0);

    assert(command_registry_execute(&editor.commands, "query-replace", &editor, false));
    assert(search_submit(&editor, MINIBUFFER_QUERY_FIND, "one"));
    assert(search_submit(&editor, MINIBUFFER_QUERY_REPLACE, "X"));
    assert(editor.minibuffer.mode == MINIBUFFER_QUERY_CONFIRM);
    SDL_Event replace = {0};
    replace.type = SDL_EVENT_KEY_DOWN;
    replace.key.key = SDLK_Y;
    assert(search_handle_confirmation(&editor, &replace));
    assert(strcmp(document->text, "X two one") == 0);
    SDL_Event stop = {0};
    stop.type = SDL_EVENT_KEY_DOWN;
    stop.key.key = SDLK_Q;
    assert(search_handle_confirmation(&editor, &stop));
    assert(editor.minibuffer.mode == MINIBUFFER_INACTIVE);
    buffers_destroy(&editor.buffers);
}

/* Verifies UTF-8 document insertion, coordinates, selection, and replacement. */
static void test_document(void)
{
    Document document;
    assert(document_init(&document));
    assert(document_insert(&document, "olá\nmundo"));
    assert(document.length == strlen("olá\nmundo"));
    assert(text_line_at(&document, document.cursor) == 1);
    assert(text_column_at(&document, document.cursor) == 5);
    document.anchor = 0;
    document.cursor = strlen("olá");
    assert(document_has_selection(&document));
    assert(document_insert(&document, "oi"));
    assert(strcmp(document.text, "oi\nmundo") == 0);
    document_destroy(&document);
}

/* Verifies reversible edits, revision dirtiness, and redo invalidation. */
static void test_undo_redo(void)
{
    Document document;
    assert(document_init(&document));
    assert(document_insert(&document, "olá\nlinha dois"));
    document_mark_clean(&document);
    assert(!document.dirty);

    document.anchor = 0;
    document.cursor = strlen("olá");
    assert(document_insert(&document, "ação"));
    assert(strcmp(document.text, "ação\nlinha dois") == 0);
    assert(document.dirty);
    assert(document_undo(&document));
    assert(strcmp(document.text, "olá\nlinha dois") == 0);
    assert(document.cursor == strlen("olá"));
    assert(document.anchor == 0);
    assert(!document.dirty);
    assert(document_redo(&document));
    assert(strcmp(document.text, "ação\nlinha dois") == 0);
    assert(document.dirty);

    assert(document_undo(&document));
    document.cursor = document.anchor = document.length;
    assert(document_insert(&document, "!"));
    assert(!document_redo(&document));
    document_destroy(&document);
}

/* Verifies adjacent typed UTF-8 input behaves as one reversible edit group. */
static void test_grouped_typing(void)
{
    Document document;
    assert(document_init(&document));
    assert(document_insert_typed(&document, "a"));
    assert(document_insert_typed(&document, "ç"));
    assert(document_insert_typed(&document, "\n"));
    assert(document_insert_typed(&document, "β"));
    assert(strcmp(document.text, "aç\nβ") == 0);
    assert(document_undo(&document));
    assert(strcmp(document.text, "") == 0);
    assert(document_redo(&document));
    assert(strcmp(document.text, "aç\nβ") == 0);

    document_mark_clean(&document);
    assert(document_insert_typed(&document, "x"));
    assert(document_undo(&document));
    assert(strcmp(document.text, "aç\nβ") == 0);
    assert(!document.dirty);
    document_destroy(&document);
}

/* Verifies atomic replacement, permissions, contents, and symlink handling. */
static void test_atomic_save(void)
{
    char directory[] = "/tmp/mt-save-test-XXXXXX";
    assert(mkdtemp(directory));
    char path[512];
    assert(snprintf(path, sizeof(path), "%s/document.txt", directory) > 0);
    FILE *file = fopen(path, "wb");
    assert(file);
    assert(fwrite("original", 1, 8, file) == 8);
    assert(fclose(file) == 0);
    assert(chmod(path, 0640) == 0);
    struct stat before;
    assert(stat(path, &before) == 0);

    Document document;
    char message[512];
    assert(document_init(&document));
    assert(document_load(&document, path, message, sizeof(message)));
    document.anchor = 0;
    document.cursor = document.length;
    assert(document_insert(&document, "conteúdo\nseguro"));
    assert(document_save(&document, message, sizeof(message)));
    assert(!document.dirty);

    struct stat information;
    assert(stat(path, &information) == 0);
    assert((information.st_mode & 0777) == 0640);
    assert(information.st_ino != before.st_ino);
    file = fopen(path, "rb");
    assert(file);
    char contents[64] = {0};
    assert(fread(contents, 1, sizeof(contents) - 1, file) ==
           strlen("conteúdo\nseguro"));
    assert(fclose(file) == 0);
    assert(strcmp(contents, "conteúdo\nseguro") == 0);

    DIR *listing = opendir(directory);
    assert(listing);
    size_t entries = 0;
    struct dirent *entry;
    while ((entry = readdir(listing)))
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            entries++;
    assert(closedir(listing) == 0);
    assert(entries == 1);

    document_destroy(&document);

    char link_path[512];
    assert(snprintf(link_path, sizeof(link_path), "%s/link.txt", directory) > 0);
    assert(symlink("document.txt", link_path) == 0);
    assert(document_init(&document));
    assert(document_load(&document, link_path, message, sizeof(message)));
    document.anchor = 0;
    document.cursor = document.length;
    assert(document_insert(&document, "via link"));
    assert(document_save(&document, message, sizeof(message)));
    assert(lstat(link_path, &information) == 0);
    assert(S_ISLNK(information.st_mode));
    file = fopen(path, "rb");
    assert(file);
    memset(contents, 0, sizeof(contents));
    assert(fread(contents, 1, sizeof(contents) - 1, file) == strlen("via link"));
    assert(fclose(file) == 0);
    assert(strcmp(contents, "via link") == 0);
    document_destroy(&document);

    assert(unlink(link_path) == 0);
    assert(unlink(path) == 0);
    assert(rmdir(directory) == 0);
}

/* Verifies default bindings, overrides, modifier normalization, and file loading. */
static void test_keymap(void)
{
    assert(SDL_GetKeyFromName("x") == SDLK_X);
    assert(SDL_GetKeyFromName("right") == SDLK_RIGHT);
    Keymap keymap;
    keymap_init_default(&keymap);
    SDL_KeyboardEvent event = {.key = SDLK_S, .mod = SDL_KMOD_CTRL};
    assert(strcmp(keymap_lookup(&keymap, &event), "save") == 0);
    event.mod = SDL_KMOD_LCTRL;
    assert(strcmp(keymap_lookup(&keymap, &event), "save") == 0);
    event.mod = SDL_KMOD_RCTRL;
    assert(strcmp(keymap_lookup(&keymap, &event), "save") == 0);
    event.key = SDLK_LEFT;
    event.mod = SDL_KMOD_LSHIFT;
    assert(strcmp(keymap_lookup(&keymap, &event), "cursor-left") == 0);
    event.key = SDLK_X;
    event.mod = SDL_KMOD_LALT;
    assert(strcmp(keymap_lookup(&keymap, &event), "execute-command") == 0);
    event.key = SDLK_T;
    assert(strcmp(keymap_lookup(&keymap, &event), "cmd") == 0);
    event.key = SDLK_RIGHT;
    event.mod = SDL_KMOD_LCTRL | SDL_KMOD_LSHIFT;
    assert(strcmp(keymap_lookup(&keymap, &event), "word-right") == 0);
    assert(keymap_bind(&keymap, SDLK_S, SDL_KMOD_CTRL, "quit"));
    event.key = SDLK_S;
    event.mod = SDL_KMOD_CTRL;
    assert(strcmp(keymap_lookup(&keymap, &event), "quit") == 0);
}

/* Verifies buffer creation, text replacement, switching, and directory entries. */
static void test_buffers(void)
{
    BufferManager buffers;
    assert(buffers_init(&buffers));
    assert(buffers.count == 1);
    Buffer *output =
        buffers_open_text(&buffers, "*test*", BUFFER_SHELL, "resultado\n", true);
    assert(output);
    assert(output->read_only);
    assert(strcmp(output->document.text, "resultado\n") == 0);
    buffers_next(&buffers);
    Buffer *scratch = buffers_current(&buffers);
    assert(strcmp(scratch->name, "*scratch*") == 0);
    assert(document_insert(&scratch->document, "rascunho"));
    buffers_next(&buffers);
    assert(!document_undo(&output->document));
    buffers_next(&buffers);
    assert(document_undo(&scratch->document));
    assert(strcmp(scratch->document.text, "") == 0);
    buffers_destroy(&buffers);
}

/* Verifies same-name file replacement cannot silently discard modified contents. */
static void test_modified_buffer_protection(void)
{
    char root[] = "/tmp/mt-buffer-test-XXXXXX";
    assert(mkdtemp(root));
    char first_directory[512], second_directory[512];
    char first_path[512], second_path[512];
    assert(snprintf(first_directory, sizeof(first_directory), "%s/first", root) > 0);
    assert(snprintf(second_directory, sizeof(second_directory), "%s/second", root) > 0);
    assert(mkdir(first_directory, 0700) == 0);
    assert(mkdir(second_directory, 0700) == 0);
    assert(snprintf(first_path, sizeof(first_path), "%s/same.txt", first_directory) >
           0);
    assert(snprintf(second_path, sizeof(second_path), "%s/same.txt", second_directory) >
           0);
    FILE *file = fopen(first_path, "wb");
    assert(file);
    assert(fwrite("first", 1, 5, file) == 5);
    assert(fclose(file) == 0);
    file = fopen(second_path, "wb");
    assert(file);
    assert(fwrite("second", 1, 6, file) == 6);
    assert(fclose(file) == 0);

    BufferManager buffers;
    char message[256];
    assert(buffers_init(&buffers));
    Buffer *buffer = buffers_open_file(&buffers, first_path, message, sizeof(message));
    assert(buffer);
    buffer->document.cursor = buffer->document.anchor = buffer->document.length;
    assert(document_insert(&buffer->document, " modified"));
    assert(buffers_modified_count(&buffers) == 1);
    assert(buffers_open_file(&buffers, first_path, message, sizeof(message)) == buffer);
    assert(strcmp(buffer->document.text, "first modified") == 0);

    assert(buffers_file_would_replace_modified(&buffers, second_path));
    assert(!buffers_open_file(&buffers, second_path, message, sizeof(message)));
    assert(strcmp(buffer->document.text, "first modified") == 0);
    assert(buffers_open_file_confirmed(&buffers, second_path, message,
                                       sizeof(message)) == buffer);
    assert(strcmp(buffer->document.text, "second") == 0);
    assert(strcmp(buffer->document.path, second_path) == 0);
    assert(buffers_modified_count(&buffers) == 0);
    buffers_destroy(&buffers);

    assert(unlink(first_path) == 0);
    assert(unlink(second_path) == 0);
    assert(rmdir(first_directory) == 0);
    assert(rmdir(second_directory) == 0);
    assert(rmdir(root) == 0);
}

/* Verifies representative C tokens produce ordered lexical highlight spans. */
static void test_highlighting(void)
{
    const char *line = "const int answer = 42; // comentário";
    HighlightSpan spans[16];
    size_t count = highlight_c_line(line, strlen(line), spans, 16);
    assert(count >= 4);
    assert(spans[0].kind == HIGHLIGHT_KEYWORD);
    assert(spans[count - 1].kind == HIGHLIGHT_COMMENT);
}

/* Runs the complete deterministic unit and controller test suite. */
int main(void)
{
    test_command_registry();
    test_settings_defaults();
    test_editing_controller();
    test_search_controller();
    test_file_controller();
    test_dired_controller();
    test_process_execution();
    test_shell_controller();
    test_document();
    test_undo_redo();
    test_grouped_typing();
    test_atomic_save();
    test_keymap();
    test_buffers();
    test_modified_buffer_protection();
    test_highlighting();
    puts("todos os testes passaram");
    return 0;
}
