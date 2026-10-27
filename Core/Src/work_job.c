#include "work_job.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *spaces(const char *p) { while (*p && isspace((unsigned char)*p)) ++p; return p; }
static CommandType_t bad(char *out, size_t n, const char *message) {
    if (out && n) snprintf(out, n, "%s", message);
    return COMMAND_ERROR;
}
const char *Work_Name(WorkKind_t kind) {
    switch (kind) { case JOB_STATS: return "stats"; case JOB_SORT: return "sort";
    default: return "unknown"; }
}
CommandType_t Work_Parse(const char *line, WorkJob_t *job, char *error, size_t n) {
    if (!line || !job) return bad(error, n, "invalid argument");
    memset(job, 0, sizeof(*job));
    const char *p = spaces(line), *start = p;
    while (*p && !isspace((unsigned char)*p)) ++p;
    size_t len = (size_t)(p - start);
    p = spaces(p);
    if (!len) return bad(error, n, "empty command");
    if (len == 4 && !strncmp(start, "help", 4))
        return *p ? bad(error, n, "help takes no arguments") : COMMAND_HELP;
    if (len == 6 && !strncmp(start, "status", 6))
        return *p ? bad(error, n, "status takes no arguments") : COMMAND_STATUS;
    if (len == 5 && !strncmp(start, "stats", 5)) job->kind = JOB_STATS;
    else if (len == 4 && !strncmp(start, "sort", 4)) job->kind = JOB_SORT;
    else return bad(error, n, "unknown command; type help");
    while (*p) {
        if (job->count == WORK_MAX_VALUES) return bad(error, n, "at most 16 integers");
        char *end; errno = 0;
        long value = strtol(p, &end, 10);
        if (p == end || errno == ERANGE || value < INT32_MIN || value > INT32_MAX ||
            (*end && !isspace((unsigned char)*end)))
            return bad(error, n, "expected signed 32-bit decimal integers");
        job->data.values[job->count++] = (int32_t)value;
        p = spaces(end);
    }
    if (!job->count) return bad(error, n, "provide 1..16 integers");
    return COMMAND_JOB;
}
void Work_Execute(const WorkJob_t *job, WorkResult_t *r) {
    memset(r, 0, sizeof(*r));
    if (job->kind == JOB_STATS || job->kind == JOB_SORT) {
        r->min = r->max = job->data.values[0];
        for (uint32_t i = 0; i < job->count; ++i) {
            int32_t v = job->data.values[i]; r->sum += v; r->sorted[i] = v;
            if (v < r->min) r->min = v;
            if (v > r->max) r->max = v;
        }
        if (job->kind == JOB_SORT) {
            for (uint32_t i = 1; i < job->count; ++i) {
                int32_t v = r->sorted[i]; uint32_t j = i;
                while (j && r->sorted[j-1U] > v) { r->sorted[j] = r->sorted[j-1U]; --j; }
                r->sorted[j] = v;
            }
        }
    }
}
/* Avoid newlib-nano's optional long-long printf support and avoid floating point. */
static void i64_text(int64_t value, char *out) {
    char reversed[21]; unsigned n = 0, pos = 0;
    uint64_t v = value < 0 ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;
    do { reversed[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    if (value < 0) out[pos++] = '-';
    while (n) out[pos++] = reversed[--n];
    out[pos] = '\0';
}
int Work_Format(const WorkJob_t *job, const WorkResult_t *r, char *out, size_t size) {
    if (job->kind == JOB_STATS) {
        char sum[22], whole[22]; i64_text(r->sum, sum);
        uint64_t magnitude = r->sum < 0 ? (uint64_t)(-r->sum) : (uint64_t)r->sum;
        i64_text((int64_t)(magnitude / job->count), whole);
        unsigned fraction = (unsigned)((magnitude % job->count) * 1000U / job->count);
        return snprintf(out, size, "n=%lu min=%ld max=%ld sum=%s mean=%s%s.%03u",
            (unsigned long)job->count, (long)r->min, (long)r->max, sum,
            r->sum < 0 ? "-" : "", whole, fraction);
    }
    if (job->kind == JOB_SORT) {
        size_t used = 0;
        for (uint32_t i = 0; i < job->count; ++i) {
            if (used >= size) return -1;
            int written = snprintf(out + used, size - used, "%s%ld", i ? " " : "", (long)r->sorted[i]);
            if (written < 0 || (size_t)written >= size - used) return -1;
            used += (size_t)written;
        }
        return (int)used;
    }
    return -1;
}
