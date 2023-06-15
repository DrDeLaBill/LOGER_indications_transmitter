#include "utils.h"

#include <stdio.h>
#include "stm32f4xx_hal.h"


void Util_TimerStart(dio_timer_t* tm, uint32_t waitMs) {
	tm->start = HAL_GetTick();
	tm->delay = waitMs;
}


uint8_t Util_TimerPending(dio_timer_t* tm) {
	return (HAL_GetTick() - tm->start) < tm->delay;
}

bool wait_event(bool (*condition) (void), uint32_t time)
{
    uint32_t start_time = HAL_GetTick();
    while (ABS_DIF(start_time, HAL_GetTick()) < time) {
        if (condition()) {
            return TRUE;
        }
    }
    return FALSE;
}
