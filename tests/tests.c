#include "buffer.h"
#include "command.h"
#include "document.h"
#include "highlight.h"
#include "keymap.h"
#include "text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static void test_keymap(void)
{
    assert(SDL_GetKeyFromName("x") == SDLK_X);
    assert(SDL_GetKeyFromName("right") == SDLK_RIGHT);
    assert(command_from_name("cmd") == COMMAND_SHELL);
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
    assert(strcmp(buffers_current(&buffers)->name, "*scratch*") == 0);
    buffers_destroy(&buffers);
}

static void test_highlighting(void)
{
    const char *line = "const int answer = 42; // comentário";
    HighlightSpan spans[16];
    size_t count = highlight_c_line(line, strlen(line), spans, 16);
    assert(count >= 4);
    assert(spans[0].kind == HIGHLIGHT_KEYWORD);
    assert(spans[count - 1].kind == HIGHLIGHT_COMMENT);
}

int main(void)
{
    test_document();
    test_keymap();
    test_buffers();
    test_highlighting();
    puts("todos os testes passaram");
    return 0;
}
