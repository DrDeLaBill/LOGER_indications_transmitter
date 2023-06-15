#ifndef INC_UTILS_H_
#define INC_UTILS_H_


#include <stdio.h>
#include <stdbool.h>


#ifndef __min
#define __min(x, y) ((x) < (y) ? (x) : (y))
#endif


#ifndef __max
#define __max(x, y) ((x) > (y) ? (x) : (y))
#endif


#ifndef __abs
#define __abs(v) (((v) < 0) ? (-(v)) : (v))
#endif


#ifndef __sgn
#define __sgn(v) (((v) > 0) ? 1 : -1)
#endif


typedef struct _dio_timer_t {
	uint32_t start;
	uint32_t delay;
} dio_timer_t;

void Util_TimerStart(dio_timer_t* tm, uint32_t waitMs);
uint8_t Util_TimerPending(dio_timer_t* tm);

bool wait_event(bool (*condition) (void), uint32_t time);


#endif
