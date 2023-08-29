#ifndef INC_RECORDMANAGER_H_
#define INC_RECORDMANAGER_H_


#include <stdint.h>

#include "main.h"
#include "storage_data_manager.h"


#define RECORD_BEDUG (true)


class RecordManager {

public:
	typedef enum _sensor_status_t {
		RECORD_OK = 0,
		RECORD_ERROR,
		RECORD_EMPTY
	} record_status_t;

	typedef struct __attribute__((packed)) _record_t {
		uint32_t record_id;
		uint32_t record_time;
		uint8_t  sensors_statuses[LOW_MB_ARR_SIZE];
		int16_t  sensors_values[LOW_MB_ARR_SIZE];
	} record_t;

private:
	static const char* MODULE_TAG;

public:
	static record_t record;

	RecordManager();
    static record_status_t load(uint32_t record_id);
	static record_status_t save();
	static record_status_t get_new_record_id(uint32_t* new_id);

};


#endif
