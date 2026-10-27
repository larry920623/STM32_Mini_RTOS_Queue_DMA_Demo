#include "work_job.h"
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    WorkJob_t job; WorkResult_t result; char out[256], error[80];
    if (Work_Parse(argv[1], &job, error, sizeof(error)) != COMMAND_JOB) { puts(error); return 1; }
    Work_Execute(&job, &result);
    if (Work_Format(&job, &result, out, sizeof(out)) < 0) return 3;
    puts(out); return 0;
}
