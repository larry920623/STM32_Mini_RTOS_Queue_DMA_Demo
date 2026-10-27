#include "work_job.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static WorkJob_t parse(const char *s) {
    WorkJob_t job; char error[80];
    assert(Work_Parse(s, &job, error, sizeof(error)) == COMMAND_JOB);
    return job;
}
static void expect(const char *command, const char *expected) {
    WorkJob_t job = parse(command); WorkResult_t result; char out[256];
    Work_Execute(&job, &result);
    assert(Work_Format(&job, &result, out, sizeof(out)) >= 0);
    if (strcmp(out, expected)) { fprintf(stderr, "%s: got [%s], want [%s]\n", command, out, expected); assert(0); }
}
int main(void) {
    expect("stats 10 20 30", "n=3 min=10 max=30 sum=60 mean=20.000");
    expect("stats -1 0 0", "n=3 min=-1 max=0 sum=-1 mean=-0.333");
    expect("stats 2147483647 2147483647", "n=2 min=2147483647 max=2147483647 sum=4294967294 mean=2147483647.000");
    expect("stats -2147483648 -2147483648", "n=2 min=-2147483648 max=-2147483648 sum=-4294967296 mean=-2147483648.000");
    expect("sort 9 -3 4 4 0", "-3 0 4 4 9");
    expect("sort -2147483648 2147483647", "-2147483648 2147483647");
    WorkJob_t job; char error[80];
    const char *bad[] = {"", " ", "stats", "sort", "stats 1x", "stats 2147483648", "stats -2147483649",
        "stats +", "stats 0x10", "bench", "bench 0", "bench -1", "bench 4097", "bench 2 3", "bench 1", "crc32 123456789", "crc32", "help x", "status x", "foo"};
    for (unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i)
        assert(Work_Parse(bad[i], &job, error, sizeof(error)) == COMMAND_ERROR);
    assert(Work_Parse("help", &job, error, sizeof(error)) == COMMAND_HELP);
    assert(Work_Parse("status  ", &job, error, sizeof(error)) == COMMAND_STATUS);
    char line[256]="stats";
    for (unsigned i=0;i<16;++i) strcat(line," 1");
    assert(Work_Parse(line,&job,error,sizeof(error))==COMMAND_JOB && job.count==16);
    strcat(line," 1"); assert(Work_Parse(line,&job,error,sizeof(error))==COMMAND_ERROR);
    job=parse("sort 3 2 1");WorkResult_t r;Work_Execute(&job,&r);
    char tiny[2];assert(Work_Format(&job,&r,tiny,sizeof(tiny))<0);
    puts("PASS: workbench algorithms, parser limits, invalid inputs and formatting");
}
