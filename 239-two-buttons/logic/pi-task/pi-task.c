#include "pi-task/pi-task.h"
#include "systime/systime.h"
// 64 * approximately 4.7 us = 301 us at 125 MHz, below the 1 ms budget.
static const uint32_t PI_TERMS_PER_CALL=64;
static pi_state_t state=PI_NOT_STARTED;
static uint32_t terms, index;
static double sum, sign;
static uint64_t started_us, elapsed_ms;
bool pi_task_start(uint32_t count)
{
    if (!count || state==PI_RUNNING) return false;
    terms=count; index=0; sum=0; sign=1;
    started_us=systime_us(); elapsed_ms=0; state=PI_RUNNING;
    return true;
}
void pi_task_handle(void)
{
    if (state!=PI_RUNNING) return;
    uint32_t batch=terms-index;
    if (batch>PI_TERMS_PER_CALL) batch=PI_TERMS_PER_CALL;
    for (uint32_t n=0; n<batch; ++n) {
        // Convert before multiplying: 2*index must not overflow uint32_t.
        sum+=sign/(2.0*(double)index+1.0);
        sign=-sign;
        ++index;
    }
    if (index==terms) {
        elapsed_ms=(systime_us()-started_us)/1000;
        state=PI_READY;
    }
}
pi_result_t pi_task_get_result(void)
{
    return (pi_result_t){.state=state, .value=4.0*sum, .elapsed_ms=elapsed_ms};
}
