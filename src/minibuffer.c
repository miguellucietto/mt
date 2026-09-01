#include "minibuffer.h"
#include "text.h"

#include <stdio.h>
#include <string.h>

void minibuffer_open(Minibuffer *minibuffer, MinibufferMode mode, const char *prompt)
{
    minibuffer->mode = mode;
    minibuffer->length = 0;
    minibuffer->input[0] = '\0';
    snprintf(minibuffer->prompt, sizeof(minibuffer->prompt), "%s", prompt);
}

void minibuffer_close(Minibuffer *minibuffer)
{
    minibuffer->mode = MINIBUFFER_INACTIVE;
    minibuffer->length = 0;
    minibuffer->input[0] = '\0';
}

void minibuffer_insert(Minibuffer *minibuffer, const char *text)
{
    size_t length = strlen(text);
    if (minibuffer->length + length >= sizeof(minibuffer->input))
        return;
    memcpy(minibuffer->input + minibuffer->length, text, length + 1);
    minibuffer->length += length;
}

void minibuffer_backspace(Minibuffer *minibuffer)
{
    minibuffer->length = text_previous_codepoint(minibuffer->input, minibuffer->length);
    minibuffer->input[minibuffer->length] = '\0';
}
