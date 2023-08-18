/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _FLASH_STORAGE_H_
#define _FLASH_STORAGE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define FLASH_BEDUG (true)


typedef enum _flash_status_t {
	FLASH_OK    = ((uint8_t)0x00),
	FLASH_ERROR = ((uint8_t)0x01),
	FLASH_BUSY  = ((uint8_t)0x02),
	FLASH_OOM   = ((uint8_t)0x03)  // Out Of Memory
} flash_status_t;


typedef struct _flash_w25qxx_info_t {
	bool     initialized;
	bool     is_24bit_address;

	uint16_t page_size;
	uint32_t pages_count;

	uint16_t sector_size;
	uint16_t sectors_count;

	uint32_t block_size;
	uint16_t blocks_count;
} flash_w25qxx_info_t;


extern flash_w25qxx_info_t flash_w25qxx_info;


flash_status_t flash_w25qxx_init();
flash_status_t flash_w25qxx_reset();
flash_status_t flash_w25qxx_read(uint32_t addr, uint8_t* data, uint32_t len);
flash_status_t flash_w25qxx_write(uint32_t addr, uint8_t* data, uint32_t len);


#ifdef __cplusplus
}
#endif


#endif
