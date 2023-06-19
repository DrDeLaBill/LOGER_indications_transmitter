/*
 * internal_storage.h
 *
 *  Created on: 2 ���. 2022 �.
 *      Author: gauss
 */

#ifndef INC_INTERNAL_STORAGE_H_
#define INC_INTERNAL_STORAGE_H_

#include "stm32f4xx_hal.h"


#define SD_PAYLOAD_BITS_SIZE(S) ( 			\
	S 										\
	- sizeof(struct _sd_payload_header_t)	\
	- sizeof(uint16_t)						\
)

#define STORAGE_SD_PAYLOAD_MAGIC   ((uint32_t)(0xBADAC0DE))
#define STORAGE_SD_PAYLOAD_VERSION (1)


#define STORAGE_SD_MAX_PAYLOAD_SIZE 512


struct _sd_payload_header_t {
	uint32_t magic; uint8_t version;
};


#include <fatfs.h>
FRESULT intstor_read_file(const char* filename, void* buf, UINT size, UINT* br);
FRESULT intstor_read_line(const char* filename, void* buf, UINT size, UINT* br, UINT ptr);
FRESULT intstor_write_file(const char* filename, const void* buf, UINT size, UINT* bw);
FRESULT intstor_append_file(const char* filename, const void* buf, UINT size, UINT* bw);
FRESULT instor_change_file(const char* filename, const void* buf, UINT size, UINT* bw, UINT ptr);
FRESULT instor_find_file(const char* pattern);
FRESULT instor_remove_file(const char* filename);
FRESULT instor_get_free_clust(DWORD *fre_clust);


#endif /* INC_INTERNAL_STORAGE_H_ */
