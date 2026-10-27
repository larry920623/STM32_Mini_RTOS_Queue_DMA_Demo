#ifndef WORK_JOB_H
#define WORK_JOB_H
#include <stddef.h>
#include <stdint.h>
#define WORK_MAX_VALUES 16U
#define WORK_MAX_LINE 256U

typedef enum { JOB_STATS, JOB_SORT } WorkKind_t;
typedef struct {
    uint32_t id;
    WorkKind_t kind;
    uint32_t count;
    struct { int32_t values[WORK_MAX_VALUES]; } data;
} WorkJob_t;
typedef struct {
    int64_t sum;
    int32_t min, max;
    int32_t sorted[WORK_MAX_VALUES];
} WorkResult_t;
typedef enum { COMMAND_JOB, COMMAND_HELP, COMMAND_STATUS, COMMAND_ERROR } CommandType_t;
CommandType_t Work_Parse(const char *line, WorkJob_t *job, char *error, size_t error_size);
void Work_Execute(const WorkJob_t *job, WorkResult_t *result);
int Work_Format(const WorkJob_t *job, const WorkResult_t *result, char *out, size_t size);
const char *Work_Name(WorkKind_t kind);
#endif
