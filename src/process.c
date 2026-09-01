#include "process.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Writes a bounded diagnostic when provided and returns false for failure paths. */
static bool set_error(char *error, size_t error_size, const char *message)
{
    if (error && error_size)
        snprintf(error, error_size, "%s", message);
    return false;
}

bool process_run_shell(const char *command, ProcessResult *result, char *error,
                       size_t error_size)
{
    if (!command || !result)
        return set_error(error, error_size, "Invalid process arguments");
    *result = (ProcessResult){0};
    static const char prefix[] = "( ";
    static const char suffix[] = " ) 2>&1";
    const char *effective_command = *command ? command : ":";
    size_t command_length = strlen(effective_command);
    if (command_length > SIZE_MAX - sizeof(prefix) - sizeof(suffix) + 1)
        return set_error(error, error_size, "Shell command is too long");
    size_t shell_command_size = sizeof(prefix) - 1 + command_length + sizeof(suffix);
    char *shell_command = malloc(shell_command_size);
    if (!shell_command)
        return set_error(error, error_size, "Unable to allocate the shell command");
    snprintf(shell_command, shell_command_size, "%s%s%s", prefix, effective_command,
             suffix);

    FILE *pipe = popen(shell_command, "r");
    free(shell_command);
    if (!pipe)
        return set_error(error, error_size, "Unable to start the shell command");

    size_t capacity = 4096;
    size_t length = 0;
    char *output = malloc(capacity);
    if (!output) {
        pclose(pipe);
        return set_error(error, error_size, "Unable to allocate process output");
    }
    int character;
    while ((character = fgetc(pipe)) != EOF) {
        if (length + 1 >= capacity) {
            if (capacity > SIZE_MAX / 2) {
                free(output);
                pclose(pipe);
                return set_error(error, error_size, "Process output is too large");
            }
            size_t larger_capacity = capacity * 2;
            char *larger = realloc(output, larger_capacity);
            if (!larger) {
                free(output);
                pclose(pipe);
                return set_error(error, error_size, "Unable to grow process output");
            }
            output = larger;
            capacity = larger_capacity;
        }
        output[length++] = (char)character;
    }
    if (ferror(pipe)) {
        free(output);
        pclose(pipe);
        return set_error(error, error_size, "Unable to read process output");
    }
    int status = pclose(pipe);
    output[length] = '\0';
    result->output = output;
    result->status = status;
    return true;
}

void process_result_destroy(ProcessResult *result)
{
    if (!result)
        return;
    free(result->output);
    *result = (ProcessResult){0};
}
