#include "systime/systime.h"
#include "pico/time.h"
uint64_t systime_us(void) { return time_us_64(); }
uint64_t systime_ms(void) { return systime_us()/1000; }
bool systime_period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    if (!period_us) return false;
    uint64_t now=systime_us(), elapsed=now-*last_us;
    if (elapsed<period_us) return false;
    if (elapsed-period_us>=period_us) *last_us=now;
    else *last_us += period_us;
    return true;
}
