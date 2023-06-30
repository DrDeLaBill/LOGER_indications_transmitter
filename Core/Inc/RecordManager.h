#ifndef INC_RECORDMANAGER_H_
#define INC_RECORDMANAGER_H_


#include <stdint.h>

extern "C"
{
	#include "internal_storage.h"
	#include "main.h"
}


class RecordManager {

private:
	static const char* MODULE_TAG;

public:
	typedef enum _sensor_status_t {
		RECORD_OK = 0,
		RECORD_ERROR
	} record_status_t;

	// Record structure in storage
	typedef struct __attribute__((packed)) _payload_record_t {
		uint32_t record_id;
		uint32_t record_time;
		uint8_t sensors_statuses[LOW_MB_SENS_COUNT+1];
		int16_t sensors_values[LOW_MB_SENS_COUNT+1];
	} payload_record_t;

	typedef union _record_sd_payload_t {
		struct __attribute__((packed)) {
			struct _sd_payload_header_t header;
			uint8_t bits[SD_PAYLOAD_BITS_SIZE(STORAGE_SD_MAX_PAYLOAD_SIZE)];
			uint16_t crc;
		};
		struct __attribute__((packed)) {
			struct _sd_payload_header_t header;
			payload_record_t payload_record;
		} v1;
	} record_sd_payload_t;

	static const char* RECORDS_FILENAME;

	static record_sd_payload_t sd_record;

    static record_status_t load(); // TODO: продумать, что будет загружаться
	static record_status_t save();
	static void clear_buf();
	static uint32_t get_new_record_id();

};


#endif
