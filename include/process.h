#ifndef MT_PROCESS_H
#define MT_PROCESS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *output;
    int status;
} ProcessResult;

bool process_run_shell(const char *command, ProcessResult *result, char *error,
                       size_t error_size);
void process_result_destroy(ProcessResult *result);

#endif
