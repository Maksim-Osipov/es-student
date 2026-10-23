#include "profiling/profiling.h"
#if PROFILING_ENABLED
#include "systime/systime.h"
typedef struct {
    const char *name;
    uint64_t start, count, total, maximum;
    bool running;
} stopwatch_t;
static stopwatch_t watches[PROFILING_STOPWATCH_COUNT];
void profiling_stopwatch_init(unsigned number, const char *name)
{
    if (number<PROFILING_STOPWATCH_COUNT) watches[number]=(stopwatch_t){.name=name};
}
void profiling_start(unsigned number)
{
    if (number>=PROFILING_STOPWATCH_COUNT) return;
    watches[number].start=systime_us();
    watches[number].running=true;
}
void profiling_stop(unsigned number)
{
    if (number>=PROFILING_STOPWATCH_COUNT) return;
    stopwatch_t *watch=&watches[number];
    if (!watch->running) return;
    uint64_t elapsed=systime_us()-watch->start;
    watch->running=false;
    watch->total+=elapsed;
    ++watch->count;
    if (elapsed>watch->maximum) watch->maximum=elapsed;
}
bool profiling_get(unsigned number, profiling_result_t *result)
{
    if (number>=PROFILING_STOPWATCH_COUNT || !result || !watches[number].name) return false;
    const stopwatch_t *watch=&watches[number];
    *result=(profiling_result_t){.name=watch->name, .count=watch->count,
        .mean_us=watch->count ? (double)watch->total/(double)watch->count : 0,
        .max_us=watch->maximum};
    return true;
}
void profiling_reset(void)
{
    for (unsigned i=0; i<PROFILING_STOPWATCH_COUNT; ++i) {
        watches[i].count=0; watches[i].total=0; watches[i].maximum=0;
        // Discard the measurements spanning the reset command itself.
        watches[i].running=false;
    }
}
#endif
