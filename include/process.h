#ifndef MT_PROCESS_H
#define MT_PROCESS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *output;
    int status;
} ProcessResult;

/** Runs command synchronously within a strict captured-output byte limit. */
bool process_run_shell(const char *command, size_t output_limit, ProcessResult *result,
                       char *error, size_t error_size);
/** Releases output owned by result and resets all result fields. */
void process_result_destroy(ProcessResult *result);

#endif
