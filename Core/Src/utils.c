#include "utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"


void util_timer_start(dio_timer_t* timer, uint32_t delay) {
	timer->start = HAL_GetTick();
	timer->delay = delay;
}

bool util_is_timer_wait(dio_timer_t* tm) {
	return ((uint32_t)((uint32_t)HAL_GetTick() - (uint32_t)tm->start)) < ((uint32_t)tm->delay);
}

int util_convert_range(int val, int rngl1, int rngh1, int rngl2, int rngh2) {
	int range1 = __abs_dif(rngh1, rngl1);
	int range2 = __abs_dif(rngh2, rngl2);
	int delta  = __abs_dif(rngl1,   val);
	return rngl2 + ((delta * range2) / range1);
}

uint16_t util_get_crc16(uint8_t* buf, uint16_t len) {
	uint16_t crc = 0;
	for (uint16_t i = 1; i < len; i++) {
		crc = (uint8_t)(crc >> 8) | (crc << 8);
		crc ^= buf[i];
		crc ^= (uint8_t)(crc & 0xFF) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}
	return crc;
}

#ifdef DEBUG
void util_debug_hex_dump(const char* tag, const uint8_t* buf, uint16_t len) {
	const uint8_t cols_count = 16;
	for (uint32_t i = 0; i < len / cols_count; i++) {
		LOG_TAG_BEDUG(tag, "");
		LOG_BEDUG("%08x: ", i);
		for (uint32_t j = 0; j < cols_count; j++) {
			if (i + j > len) {
				break;
			}
			LOG_BEDUG("%02x ", buf[i + j]);
			if (j + 1 == 8) {
				LOG_BEDUG("| ");
			}
		}
		LOG_BEDUG("|\n");
	}
}
#else /* DEBUG */
void util_debug_hex_dump(const char* tag, const uint8_t* buf, uint16_t len) {}
#endif /* DEBUG */

bool util_wait_event(bool (*condition) (void), uint32_t time)
{
    uint32_t start_time = HAL_GetTick();
    while (__abs(start_time - HAL_GetTick()) < time) {
        if (condition()) {
            return true;
        }
    }
    return false;
}

int _write(int file, uint8_t *ptr, int len) {
	HAL_UART_Transmit(&PRINT_MESSAGE_UART, (uint8_t *)ptr, len, DEFAULT_UART_DELAY);
#ifdef DEBUG
	for (int DataIdx = 0; DataIdx < len; DataIdx++) {
		ITM_SendChar(*ptr++);
	}
	return len;
#endif
}
