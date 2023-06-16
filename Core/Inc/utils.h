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


#ifdef DEBUG
#define LOG_DEBUG(MODULE_TAG, format, ...) { \
	printf("%s:", MODULE_TAG); printf(format __VA_OPT__(,) __VA_ARGS__); \
}
#define LOG_DEBUG_LN(format, ...) { \
	printf(format __VA_OPT__(,) __VA_ARGS__);   \
}
#else /* DEBUG */
#define LOG_DEBUG(MODULE_TAG, format, ...) {}
#define LOG_DEBUG_LN(format, ...) {}
#endif /* DEBUG */


typedef struct _dio_timer_t {
	uint32_t start;
	uint32_t delay;
} dio_timer_t;

void Util_TimerStart(dio_timer_t* tm, uint32_t waitMs);
uint8_t Util_TimerPending(dio_timer_t* tm);

bool wait_event(bool (*condition) (void), uint32_t time);

void Debug_HexDump(const char* tag, const uint8_t* buf, uint16_t len);


#endif
